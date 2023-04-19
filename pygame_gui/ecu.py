import threading
import time
from threading import Thread

import pygame
import serial
from pubsub import pub
import logging

POWERTRAIN = 1
CHASSIS = 2
BODY = 3
OBSERVER = 4
GATEWAY = 5

POWERTRAIN_RECEIVED = pygame.USEREVENT + POWERTRAIN
CHASSIS_RECEIVED = pygame.USEREVENT + CHASSIS
BODY_RECEIVED = pygame.USEREVENT + BODY
OBSERVER_RECEIVED = pygame.USEREVENT + OBSERVER
GATEWAY_RECEIVED = pygame.USEREVENT + GATEWAY


class Sender:
    def __init__(self, pts, name):
        self.pts = pts
        self.name = name
        self.stopped = threading.Event()
        logging.info("Starting sender for %s on %s",
                     self.name, '/dev/pts/' + self.pts[0])
        self.ser = serial.Serial('/dev/pts/' + self.pts[0], baudrate=9600)
        pub.subscribe(self.send_command, name)
        self.thread = Thread(target=self.run)

    def run(self):
        while not self.stopped.is_set():
            time.sleep(0.01)

    def send_command(self, msg):
        msg = "EXD" + msg + '\r'
        # logging.info("%s sending: %s", self.name, str(msg))
        self.ser.write(msg.encode())


class Receiver:

    def __init__(self, pts, name, ecu_type):
        self.pts = pts
        self.name = name
        self.stopped = threading.Event()
        self.ecu_type = ecu_type
        logging.info("Starting receiver for %s on %s",
                     self.name, '/dev/pts/' + self.pts[0])
        self.ser = serial.Serial(
            '/dev/pts/' + self.pts[0], baudrate=9600, timeout=0.5)
        self.thread = Thread(target=self.run)

    def run(self):
        while not self.stopped.is_set():
            rec = ''
            counter = 0
            while not self.stopped.is_set():
                temp = self.ser.read()
                if temp == b'\n':
                    if counter == 0:
                        counter = 1
                    else:
                        break
                else:
                    rec += temp.decode("utf-8")
            if not self.stopped.is_set():
                recv_event = pygame.event.Event(
                    pygame.USEREVENT + self.ecu_type, message=rec)
                try:
                    pygame.event.post(recv_event)
                except Exception:
                    # logging.info(f"{self.name} can't post event, maybe game was closed")
                    pass
