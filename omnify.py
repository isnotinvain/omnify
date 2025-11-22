import time

import mido

from chords import Chord
from settings import CCPerChordMode, CCRangePerChordMode, DaemomnifySettings, MidiCCButton, MidiNoteButton, NotePerChordMode


def clamp_note(note):
    if note > 127:
        return (note % 12) + 108  # Clamp to highest octave
    return note


class Omnify:
    def __init__(self, scheduler, settings: DaemomnifySettings):
        self.scheduler = scheduler
        self.settings = settings
        self.current_chord = Chord.MAJOR
        self.note_on_events_of_current_chord = []
        self.last_strum_time = 0
        self.last_strum_zone = -1
        self.latch = False

        # Precompute chord range mapping
        self.chord_list = list(Chord)
        self.chord_region_size = 128 / len(self.chord_list)

    def stop_notes_of_current_chord(self):
        events = []
        for note in self.note_on_events_of_current_chord:
            events.append(mido.Message("note_off", note=note.note, velocity=note.velocity, channel=note.channel))
        self.note_on_events_of_current_chord.clear()
        return events

    def handle_control_note(self, msg):
        match self.settings.latch_toggle_button:
            case MidiNoteButton() as b if b.note == msg.note:
                self.latch = not self.latch
                if not self.latch:
                    return self.stop_notes_of_current_chord()
                return []

        match self.settings.stop_button:
            case MidiNoteButton() as b if b.note == msg.note:
                return self.stop_notes_of_current_chord()

        match self.settings.chord_midi_map_style:
            case NotePerChordMode() as m:
                if msg.note in m.note_mapping:
                    self.current_chord = m.note_mapping[msg.note]
                    return []

        return []

    def handle_note_on(self, msg):
        if self.settings.is_note_control_note(msg.note):
            return self.handle_control_note(msg)

        # stop currently plaing chord
        events = self.stop_notes_of_current_chord()

        # now we generate the chord note on events
        # (offsets includes 0, so no need to add msg itself,
        # and it might have the wrong channel anyway)
        root = msg.note
        for offset in self.current_chord.value.offsets:
            on = mido.Message("note_on", note=clamp_note(root + offset), velocity=msg.velocity, channel=self.settings.chord_channel - 1)
            events.append(on)
            self.note_on_events_of_current_chord.append(on)

        return events

    def handle_note_off(self, msg):
        if self.settings.is_note_control_note(msg.note):
            return []

        if self.note_on_events_of_current_chord and self.note_on_events_of_current_chord[0].note == msg.note:
            # we have released the root of the current chord
            # either stop the chord now or do nothing depending on hold mode
            if not self.latch:
                return self.stop_notes_of_current_chord()

        return []

    def handle_control_change(self, msg):
        if msg.is_cc(self.settings.strum_plate_cc):
            return self.handle_strum(msg)

        match self.settings.latch_toggle_button:
            case MidiCCButton() as b if b.cc == msg.control:
                if b.is_toggle:
                    self.latch = msg.value >= 64
                else:
                    self.latch = not self.latch
                if not self.latch:
                    return self.stop_notes_of_current_chord()
                return []

        match self.settings.stop_button:
            case MidiCCButton() as b if b.cc == msg.control:
                return self.stop_notes_of_current_chord()

        match self.settings.chord_midi_map_style:
            case CCPerChordMode() as m:
                if msg.control in m.cc_mapping:
                    self.current_chord = m.cc_mapping[msg.control]
                    return []
            case CCRangePerChordMode() as m:
                if msg.control == m.cc:
                    chord_index = min(int(msg.value / self.chord_region_size), len(self.chord_list) - 1)
                    self.current_chord = self.chord_list[chord_index]
                    return []
        return []

    def handle_strum(self, msg):
        if not self.note_on_events_of_current_chord:
            # Theres no current chord, so we don't do anything
            return []

        # we be strummin
        events = []

        now = time.perf_counter()
        cool_down_ready = now >= self.last_strum_time + self.settings.strum_cooldown_secs

        # each zone is 13/128ths in size (plus some floor-ing from integer division)
        strum_plate_zone = int((msg.value * 13) / 128)

        root = self.note_on_events_of_current_chord[0].note % 12
        velocity = self.note_on_events_of_current_chord[0].velocity

        if strum_plate_zone != self.last_strum_zone or cool_down_ready:
            # we'll allow it, we've crossed into the next strumming area or it's been long enough
            note_to_play = self.current_chord.value.arp[root][strum_plate_zone]
            events.append(mido.Message("note_on", note=note_to_play, velocity=velocity, channel=self.settings.strum_channel - 1))
            self.scheduler.schedule(
                mido.Message("note_off", note=note_to_play, channel=self.settings.strum_channel - 1),
                self.settings.strum_gate_time_secs,
            )
            self.last_strum_time = now
            self.last_strum_zone = strum_plate_zone

        return events

    def handle_incoming_message(self, msg):
        match msg.type:
            case "note_on":
                if msg.velocity == 0:
                    # dumb dumb dumb
                    return self.handle_note_off(msg)
                else:
                    return self.handle_note_on(msg)
            case "note_off":
                return self.handle_note_off(msg)
            case "control_change":
                return self.handle_control_change(msg)
            case _:
                return []
