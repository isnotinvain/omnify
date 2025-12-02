# TODO: Replace this whole thing with a UI
# TODO: Why did I even build this?? I know how to edit json files manually...

import sys
import time
from textwrap import dedent

import mido

from daemomnify.chords import ChordQuality
from daemomnify.settings import (
    DEFAULT_SETTINGS,
    CCPerChordQuality,
    CCRangePerChordQuality,
    DaemomnifySettings,
    MidiButton,
    MidiCCButton,
    MidiNoteButton,
    NotePerChordQuality,
    save_settings,
)
from daemomnify.util import irange


def select_int(r: range[int], default: int | None = None) -> int:
    if default is not None and default not in r:
        raise ValueError(f"Default provided is outside the acceptable range! Default: {default}, range: {range}")

    while True:
        try:
            default_prompt = ""
            if default is not None:
                default_prompt = f" (default: {default}, press enter to accept)"
            choice = input(f"Please enter a number: [{r.start} - {r.stop - 1}]{default_prompt}:\n")
            if len(choice) == 0 and default is not None:
                return default
            choice = int(choice)
            if choice in r:
                return choice
        except ValueError:
            print("Please enter a valid number")
        except KeyboardInterrupt:
            print("Cancelled.")
            sys.exit(0)


def select_device():
    devices = mido.get_input_names() or []

    if not devices:
        print("No MIDI input devices found! Plug in / setup some midi controllers and try again.")
        sys.exit(1)

    print("Available MIDI input devices:")
    for idx, name in enumerate(devices, 1):
        print(f"  {idx}. {name}")

    choice = select_int(irange(1, len(devices)))
    return devices[choice - 1]


def get_next_note_on(inport) -> int:
    # first clear anything queued up in inport
    for _ in inport.iter_pending():
        pass

    # now block until we get a note on
    for msg in inport:
        if msg.type == "note_on" and msg.velocity > 0:
            return msg.note


def get_next_cc(inport) -> int:
    # first clear anything queued up in inport
    for _ in inport.iter_pending():
        pass

    # now block until we get a note on
    for msg in inport:
        if msg.is_cc():
            time.sleep(0.5)
            return msg.control


# TODO the latch stuff here makes no sense for the silence button
def select_midi_button(inport) -> MidiButton:
    print(
        dedent("""
        There are 2 ways you can set this up:

        1. Use a note, usually a drum pad in a very low octave.
            Eg, pressing C-2
        2. Use a cc signal (value ignored)
            This option supports drum pads that can act as a "toggle" -- eg they send a 
            high value for on and a low value for off. Many drum pads are "momentary" though
            and send one value when pressed and another when released. You'll have to pick which you want.
    """).strip()
    )

    choice = select_int(irange(1, 2))
    match choice:
        case 1:
            print("Please press the note you want to use:")
            note = get_next_note_on(inport)
            # TODO ensure note isn't used elsewhere in settings
            return MidiNoteButton(note=note)
        case 2:
            print("Do you want to use 1. momentary or 2. toggle mode for this cc button?")
            momentary_or_toggle = select_int(irange(1, 2))
            is_toggle = momentary_or_toggle == 2
            print("Please press the cc button you want to use:")
            cc = get_next_cc(inport)
            return MidiCCButton(cc=cc, is_toggle=is_toggle)


def run_wizard():
    new_settings = DaemomnifySettings.model_construct(**DEFAULT_SETTINGS.model_dump())
    print("Let's get you setup. Make sure you've plugged in your midi controller before we contiue. Ready? [y/n]")
    response = input()

    if response != "y":
        print("ok bye!")
        sys.exit(0)

    new_settings.midi_device_name = select_device()

    print("Daemomnify outputs two separate midi channels. One for the chords that play when you hold a note, and one for the auto-harp strums.")
    print("What channel do you want to send the chords on?")
    new_settings.chord_channel = select_int(irange(1, 16), default=new_settings.chord_channel)
    print("What channel do you want to send the strums on?")
    new_settings.strum_channel = select_int(irange(1, 16), default=new_settings.strum_channel)
    # TODO -- use the midi wiggle approach
    print("What midi cc do you want to act as the strum plate? 1 is usually the mod wheel.")
    new_settings.strum_plate_cc = select_int(irange(0, 127), default=new_settings.strum_plate_cc)

    print("Now some settings you probably don't need to change, and can just keep the defaults.")
    print("How long do you want to wait between strums before sending a repeat of the same note? (in ms)")
    new_settings.strum_cooldown_secs = float(select_int(irange(0, 1000), default=int(new_settings.strum_cooldown_secs * 1000))) / 1000.0
    print("What gate time do you want to use for each pluck of the strum plate? (in ms)")
    new_settings.strum_gate_time_secs = float(select_int(irange(0, 1000), default=int(new_settings.strum_gate_time_secs * 1000))) / 1000.0

    with mido.open_input(new_settings.midi_device_name) as inport:
        print("Now we need to configure how you will choose what kind of chord to play (Major, Minor, etc).")
        print(
            dedent("""
            There are 3 ways you can set this up:

            1. Use 1 note for each chord type, usually a drum pad in a very low octave.
               Eg, pressing C-2 means Major, C#-2 means Minor, etc.
            2. Use 1 cc signal for each chord type, (value of the cc is ignored).
               Eg, multiple drum pads that each send a different cc signal, or multiple separate knobs you will wiggle to choose.
            3. Use 1 cc signal for all the chord types, and the value of the cc determines which chord type.
               The chord types are split evenly over the range 0 to 127.
        """).strip()
        )

        choice = select_int(irange(1, 3))

        match choice:
            case 1:
                note_mapping = {}
                for chord in ChordQuality:
                    while True:
                        print(f"Please press the note to switch into {chord.value.nice_name} mode:")
                        note = get_next_note_on(inport)
                        if note in note_mapping:
                            print("You alredy used that note!")
                        else:
                            note_mapping[note] = chord
                            break
                new_settings.chord_quality_selection_style = NotePerChordQuality(note_mapping=note_mapping)
            case 2:
                cc_mapping = {}
                for chord in ChordQuality:
                    while True:
                        print(f"Please press / slide / wiggle the controller to use for {chord.value.nice_name} mode:")
                        cc = get_next_cc(inport)
                        if cc in cc_mapping:
                            print("You alredy used that cc!")
                        else:
                            cc_mapping[cc] = chord
                            break
                new_settings.chord_quality_selection_style = CCPerChordQuality(cc_mapping=cc_mapping)
            case 3:
                print("Please press / slide / wiggle the controller to use for all chords:")
                cc = get_next_cc(inport)
                new_settings.chord_quality_selection_style = CCRangePerChordQuality(cc=cc)

        print("Now we need to configure how you will switch between latch / unlatched chord mode.")
        new_settings.latch_toggle_button = select_midi_button(inport)
        print("Now we need to configure how you will silence any ongoing chords.")
        new_settings.stop_button = select_midi_button(inport)

        validated = DaemomnifySettings(**new_settings.model_dump())
        save_settings(validated)
        print("Alright, you're ready to go! Please relaunch Daemomnify.")
