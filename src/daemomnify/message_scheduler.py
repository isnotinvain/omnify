import heapq
import time
import traceback
from dataclasses import dataclass, field
from typing import Any


@dataclass(order=True)
class ScheduledMessage:
    send_time: int
    msg: Any = field(compare=False)


class MessageScheduler:
    def __init__(self):
        self.queue = []

    def schedule(self, msg, delay_seconds):
        send_time = time.perf_counter() + delay_seconds
        heapq.heappush(self.queue, ScheduledMessage(send_time, msg))

    def send_overdue_messages(self, output_port):
        current_time = time.perf_counter()

        while self.queue and self.queue[0].send_time <= current_time:
            scheduled = heapq.heappop(self.queue)
            try:
                output_port.send(scheduled.msg)
            except OSError as e:
                print(f"Error sending scheduled message {scheduled.msg}: {e}")
                traceback.print_exc()
