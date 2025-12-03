#!/usr/bin/env python3

import time
from multiprocessing import Process

from daemomnify.daemomnify import main


def run_forever():
    crash_times = []
    max_crashes = 5
    window_seconds = 60

    while True:
        p = Process(target=main)
        p.start()
        try:
            p.join()
        except KeyboardInterrupt:
            p.terminate()
            p.join()
            print("\nShutting down.")
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
    run_forever()
