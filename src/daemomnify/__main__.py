#!/usr/bin/env python3

import sys
import time
import traceback

import mido

from daemomnify.event_dispatcher import EventDispatcher
from daemomnify.message_scheduler import MessageScheduler
from daemomnify.omnify import Omnify
from daemomnify.settings import load_settings
from daemomnify.wizard import run_wizard


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
