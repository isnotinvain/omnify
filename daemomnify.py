#!/usr/bin/env python3

# TODO: Investigate PyInstaller
#
# TODO: Add a mode that works with *samples* of real omnichord chords from omni-84 / decent sampler
# TODO: This would work by only sending one note on/off, in the correct octave for omni-84's layout
#
# TODO: Add a mode for just using the omni-84 layout, this was the first way I had thought to do the layout
# TODO: and I feel like some users would want that if they've got a big keyboard.
#
# TODO: Add a mode where the strumplate works regardless of whether a chord is playing, so that the strum
# TODO: plate can be used to drive omni-84's SonicStrings (or anything else) with or without backing chords
#
# TODO: Janky as it is, I think there is value in adding a VST UI for this, that speaks over some protocol to
# TODO: the daemon, so that all the settings can be done in-DAW, and parameters can be midi-mapped
#
# TODO: Verify if the chord_notes.json (and offsets derived from it) are accurate. They were
# TODO: extracted from sample .wav files and who knows how well that worked.
#
# TODO: Pitch bend mode for spring loaded pitch wheels. Roll up from center = strum up,
# TODO: ignore descent back to center. Swipe down from center = strum down, ignore on it's way back up to center
#
import sys
import time
import traceback

import mido

from event_dispatcher import EventDispatcher
from message_scheduler import MessageScheduler
from omnify import Omnify
from settings import load_settings
from wizard import run_wizard


def create_virtual_output(port_name="Daemomnify"):
    try:
        return mido.open_output(port_name, virtual=True)
    except OSError as e:
        print(f"Error creating virtual output: {e}")
        traceback.print_exc()
        return None


def main():
    print("=== Welcome to Daemomnify. Let's Omnify some instruments! ===")

    settings = load_settings()
    if not settings:
        settings = run_wizard()
        sys.exit(0)

    # Create virtual output
    print("Creating virtual MIDI output...")
    virtual_output = create_virtual_output()
    if not virtual_output:
        print("Failed to create virtual output! Dang! See ya!")
        sys.exit(1)

    print(f"Virtual output created: {virtual_output.name}")

    # Create message scheduler
    scheduler = MessageScheduler()
    event_dispatcher = EventDispatcher()

    # omnify will register itself as handlers for things
    Omnify(scheduler, event_dispatcher, settings)

    try:
        with mido.open_input(settings.midi_device_name) as inport:
            # Open input device and start listening
            print(f"Listening to {settings.midi_device_name}...")
            print("Press Ctrl+C to stop")

            while True:
                # Check for any pending incoming messages (non-blocking)
                for msg in inport.iter_pending():
                    to_send_now = event_dispatcher.handle(msg)
                    # Send immediate message to virtual output
                    try:
                        if to_send_now:
                            for m in to_send_now:
                                virtual_output.send(m)
                    except OSError as e:
                        print(f"Error sending message {m}: {e}")
                        traceback.print_exc()

                # Send any scheduled messages whose time has arrived
                scheduler.send_overdue_messages(virtual_output)

                # Sleep briefly to avoid spinning CPU (1ms = ~1000Hz poll rate)
                time.sleep(0.001)

    except KeyboardInterrupt:
        print("Stopping...")
    except Exception as e:
        print(f"Error: {e}")
        traceback.print_exc()
    finally:
        if virtual_output:
            virtual_output.close()
        print("Daemomnify banished.")


if __name__ == "__main__":
    main()
