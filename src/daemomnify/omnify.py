import time

import mido

from daemomnify.chords import ChordQuality
from daemomnify.event_dispatcher import EventDispatcher
from daemomnify.message_scheduler import MessageScheduler
from daemomnify.settings import ButtonAction, DaemomnifySettings
from daemomnify.util import clamp_note


class Omnify:
    def __init__(self, scheduler: MessageScheduler, event_dispatcher: EventDispatcher, settings: DaemomnifySettings):
        self.scheduler = scheduler
        self.settings = settings

        # we need this for when the user queues up a new chord quality, but hasn't started a new chord yet.
        # we need the strum to not change until the next chord
        self.enqueued_chord_quality = ChordQuality.MAJOR
        self.current_chord_quality = None

        self.current_root = None
        self.note_on_events_of_current_chord = []
        self.last_strum_time = 0
        self.last_strum_zone = -1
        self.latch = False

        # Buttons and control signals first so that they don't get treated as normal notes etc
        event_dispatcher.register_handler(self.settings.chord_quality_selection_style.handles_message, self.on_chord_quality_change)
        event_dispatcher.register_handler(self.settings.stop_button.handles_message, self.on_stop_button)
        event_dispatcher.register_handler(self.settings.latch_toggle_button.handles_message, self.on_latch_button)

        # now these guys don't have to filter out things that belong to the guys above
        event_dispatcher.register_handler(self.is_chord_note_on, self.on_chord_note_on)
        event_dispatcher.register_handler(self.is_chord_note_off, self.on_chord_note_off)
        event_dispatcher.register_handler(self.is_strum, self.on_strum)

    def on_chord_quality_change(self, chord):
        self.enqueued_chord_quality = chord

    def on_stop_button(self, _):
        return self.stop_notes_of_current_chord()

    def on_latch_button(self, action):
        match action:
            case ButtonAction.ON:
                self.latch = True
            case ButtonAction.OFF:
                self.latch = False
            case ButtonAction.FLIP:
                self.latch = not self.latch

        if not self.latch:
            return self.stop_notes_of_current_chord()

    def is_strum(self, msg):
        if msg.is_cc(self.settings.strum_plate_cc):
            return msg.value
        return False

    def on_strum(self, control_value):
        if not self.current_chord_quality:
            # Theres no current chord, so we don't do anything
            return

        # we be strummin
        events = []

        now = time.perf_counter()
        cool_down_ready = now >= self.last_strum_time + self.settings.strum_cooldown_secs

        # each zone is 13/128ths in size (plus some floor-ing from integer division)
        strum_plate_zone = int((control_value * 13) / 128)

        velocity = self.note_on_events_of_current_chord[0].velocity

        if strum_plate_zone != self.last_strum_zone or cool_down_ready:
            # we'll allow it, we've crossed into the next strumming area or it's been long enough

            # find the note within the arp sequence for this given root
            note_to_play = self.settings.strum_voicing_style.get(self.settings).construct_chord(self.current_chord_quality, self.current_root)[
                strum_plate_zone
            ]

            # create the note on event
            events.append(mido.Message("note_on", note=note_to_play, velocity=velocity, channel=self.settings.strum_channel - 1))

            # schedule note off in the future (respecting gate time)
            self.scheduler.schedule(
                mido.Message("note_off", note=note_to_play, channel=self.settings.strum_channel - 1),
                self.settings.strum_gate_time_secs,
            )

            # cooldown / previous zone bookkeeping
            self.last_strum_time = now
            self.last_strum_zone = strum_plate_zone

        return events

    def is_chord_note_on(self, msg):
        if msg.type == "note_on" and msg.velocity > 0:
            return msg
        return False

    def on_chord_note_on(self, msg):
        # stop currently plaing chord
        events = self.stop_notes_of_current_chord() or []

        self.current_chord_quality = self.enqueued_chord_quality
        # now we generate the chord note_on evnets
        # (chord offsets includes 0, so no need to add msg itself,
        # and it might have the wrong channel anyway)
        self.current_root = msg.note
        clamped_notes = set()
        for note in self.settings.chord_voicing_style.get(self.settings).construct_chord(self.current_chord_quality, self.current_root):
            clamped = clamp_note(note)
            if clamped in clamped_notes:
                continue
            clamped_notes.add(clamped)

            on = mido.Message("note_on", note=clamped, velocity=msg.velocity, channel=self.settings.chord_channel - 1)
            events.append(on)
            self.note_on_events_of_current_chord.append(on)

        return events

    def is_chord_note_off(self, msg):
        if msg.type == "note_off":
            return msg.note
        if msg.type == "note_on" and msg.velocity == 0:
            return msg.note
        return False

    def on_chord_note_off(self, note):
        # check if we are currently playing a chord
        if self.current_root and self.current_root == note:
            # we have released the root of the currently playing chord
            # (we want to ignore note off for other notes, they might be held from before)
            # either stop the chord now or do nothing depending on latch mode
            if not self.latch:
                return self.stop_notes_of_current_chord()

    def stop_notes_of_current_chord(self):
        self.current_chord_quality = None
        self.current_root = None
        if not self.note_on_events_of_current_chord:
            return

        events = []
        for note in self.note_on_events_of_current_chord:
            events.append(mido.Message("note_off", note=note.note, velocity=note.velocity, channel=note.channel))
        self.note_on_events_of_current_chord.clear()
        return events
