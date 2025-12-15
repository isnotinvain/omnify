#!/usr/bin/env python3

import argparse
import multiprocessing
import time

from daemomnify.daemomnify import EXIT_CODE_QUIT, main


def run_forever(osc_port: int | None = None, temp_dir: str | None = None):
    crash_times = []
    max_crashes = 5
    window_seconds = 60

    while True:
        p = multiprocessing.Process(target=main, args=(osc_port, temp_dir))
        p.start()
        try:
            p.join()
        except KeyboardInterrupt:
            p.terminate()
            p.join()
            print("\nShutting down.")
            break

        # Check if this was a graceful shutdown (don't restart)
        if p.exitcode == EXIT_CODE_QUIT:
            print("Graceful shutdown complete.")
            break

        # Track crash time
        now = time.time()
        crash_times.append(now)
        # Only keep crashes within the window
        crash_times = [t for t in crash_times if now - t < window_seconds]

        if len(crash_times) >= max_crashes:
            print(f"Crashed {max_crashes} times in {window_seconds}s, giving up.")
            break

        print("Process exited, restarting...")


if __name__ == "__main__":
    # Required for PyInstaller-bundled executables to handle multiprocessing correctly
    multiprocessing.freeze_support()

    parser = argparse.ArgumentParser(description="Daemomnify - Transform MIDI instruments into omnichords")
    parser.add_argument("--osc-port", type=int, default=None, help="OSC port to listen on for settings from VST")
    parser.add_argument("--temp-dir", type=str, default=None, help="Temp directory for ready file (must match VST)")
    args = parser.parse_args()
    run_forever(osc_port=args.osc_port, temp_dir=args.temp_dir)
