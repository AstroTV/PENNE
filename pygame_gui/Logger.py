import logging
import threading
from threading import Thread
import can


class CanLogger:
    s = None

    def __init__(self):
        self.stopped = threading.Event()
        self.name = "CanLogger"
        self.thread = Thread(target=self.run)
        pass

    def run(self):
        try:
            can_interface = 'vcan0'
            bus = can.interface.Bus(can_interface, bustype='socketcan')
            f = open("can.log", "w")

            while not self.stopped.is_set():
                message = bus.recv(0.1)
                if message:
                    print(message, file=f)
        except Exception as e:
            logging.exception(
                "Can logger failed. Did you run init.sh?\nError: %s", e)
            exit(1)
