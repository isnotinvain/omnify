#!/usr/bin/env python3

import sys
import threading
import time
import traceback

import mido
from pythonosc.dispatcher import Dispatcher
from pythonosc.osc_server import BlockingOSCUDPServer

from daemomnify.event_dispatcher import EventDispatcher
from daemomnify.message_scheduler import MessageScheduler
from daemomnify.omnify import Omnify
from daemomnify.settings import load_settings
from daemomnify.wizard import run_wizard

# Global flag to signal shutdown
_shutdown_requested = threading.Event()

# Exit code to signal graceful shutdown (don't restart)
EXIT_CODE_QUIT = 42


def _handle_quit(address, *args):
    """OSC handler for /quit - signals graceful shutdown."""
    print("Received /quit OSC message, shutting down...")
    _shutdown_requested.set()


def _start_osc_server(port: int) -> BlockingOSCUDPServer:
    """Start OSC server in a background thread."""
    dispatcher = Dispatcher()
    dispatcher.map("/quit", _handle_quit)

    server = BlockingOSCUDPServer(("127.0.0.1", port), dispatcher)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    print(f"OSC server listening on 127.0.0.1:{port}")
    return server


def create_virtual_output(port_name="Daemomnify"):
    try:
        return mido.open_output(port_name, virtual=True)
    except OSError as e:
        print(f"Error creating virtual output: {e}")
        traceback.print_exc()
        return None


def run_message_loop(device_name, event_dispatcher, scheduler, virtual_output):
    with mido.open_input(device_name) as inport:
        print(f"Listening to {device_name}...")
        print("Press Ctrl+C to stop")

        while not _shutdown_requested.is_set():
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


def main(osc_port: int | None = None):
    print("=== Welcome to Daemomnify. Let's Omnify some instruments! ===")

    # Start OSC server if port specified (for VST control)
    osc_server = None
    if osc_port:
        osc_server = _start_osc_server(osc_port)

    settings = load_settings()
    if not settings:
        settings = run_wizard()
        sys.exit(0)

    # Wait for MIDI input device to become available
    if settings.midi_device_name not in mido.get_input_names():
        print(f"Waiting for MIDI device '{settings.midi_device_name}' to appear...")
        try:
            while settings.midi_device_name not in mido.get_input_names() and not _shutdown_requested.is_set():
                time.sleep(0.5)
            if _shutdown_requested.is_set():
                print("Shutdown requested while waiting for MIDI device.")
                print("Daemomnify banished.")
                sys.exit(EXIT_CODE_QUIT)
        except KeyboardInterrupt:
            print("\nStopping...")
            print("Daemomnify banished.")
            sys.exit(EXIT_CODE_QUIT)
    print(f"Found MIDI device: {settings.midi_device_name}")

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

    graceful_exit = False
    try:
        run_message_loop(settings.midi_device_name, event_dispatcher, scheduler, virtual_output)
        # If we got here, loop exited due to _shutdown_requested (OSC /quit)
        graceful_exit = True
    except KeyboardInterrupt:
        print("\nStopping...")
        graceful_exit = True
    except Exception as e:
        print(f"Error: {e}")
        traceback.print_exc()
    finally:
        if virtual_output:
            virtual_output.close()
        print("Daemomnify banished.")

    if graceful_exit:
        sys.exit(EXIT_CODE_QUIT)
