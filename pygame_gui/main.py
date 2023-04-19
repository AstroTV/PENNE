import traceback
from secrets import choice
from gui import GUI
import subprocess
import sys
import logging
from Logger import CanLogger
from ecu import *
from car import Car
from constants import BASE_PROJECT_PATH, BASE_GUI_PATH
from setup_panel import SetupPanel, MitigationType


def generate_256_bit_key() -> bytes:
    key = bytearray()
    for i in range(32):
        # We do not want 0 bytes when we pass the key over the commandline
        key.append(choice(range(1, 256)))
    return bytes(key)


def start_communication_threads(pts, selected_mitigations: [MitigationType, bool]):
    threads = []
    try:
        threads.append(Receiver(pts[2], "powertrain", POWERTRAIN))
        threads.append(Sender(pts[1], "chassis"))
        threads.append(Receiver(pts[1], "chassis", CHASSIS))
        threads.append(Receiver(pts[0], "body", BODY))

        if MitigationType.OBSERVER in selected_mitigations and selected_mitigations[MitigationType.OBSERVER]:
            assert pts[3] is not None
            threads.append(Receiver(pts[3], "observer", OBSERVER))

        if MitigationType.GATEWAY in selected_mitigations and selected_mitigations[MitigationType.GATEWAY]:
            assert pts[4] is not None
            threads.append(Receiver(pts[4], "gateway", GATEWAY))

        threads.append(CanLogger())

        for t in threads:
            t.thread.start()
    except Exception as e:
        logging.exception(e)
        threads = []

    return threads


def setup_logging(log_file="gui.log", print_to_stderr=True):
    logging.basicConfig(filename=log_file, level=logging.INFO,
                        format='%(asctime)s %(message)s')
    if print_to_stderr:
        logging.getLogger().addHandler(logging.StreamHandler())


def start_relay():
    return subprocess.Popen(['socat', '-d', '-d', 'pty,raw,echo=0',
                             'pty,raw,echo=0'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def start_socat_relays(selected_mitigations: [MitigationType, bool]):
    pts = []
    processes = []
    logging.info("Starting socat relays")

    try:
        body_relay = start_relay()
        logging.info("  Starting socat relay for Body")
        pt = (body_relay.stderr.readline().strip().split(b'/')[-1].decode(sys.stdout.encoding),
              body_relay.stderr.readline().strip().split(b'/')[-1].decode(sys.stdout.encoding))
        pts.append(pt)
        processes.append(body_relay)

        chassis_relay = start_relay()
        logging.info("  Starting socat relay for Chassis")
        pt = (chassis_relay.stderr.readline().strip().split(b'/')[-1].decode(sys.stdout.encoding),
              chassis_relay.stderr.readline().strip().split(b'/')[-1].decode(sys.stdout.encoding))
        pts.append(pt)
        processes.append(chassis_relay)

        powertrain_relay = start_relay()
        logging.info("  Starting socat relay for Powertrain")
        pt = (powertrain_relay.stderr.readline().strip().split(b'/')[-1].decode(sys.stdout.encoding),
              powertrain_relay.stderr.readline().strip().split(b'/')[-1].decode(sys.stdout.encoding))
        pts.append(pt)
        processes.append(powertrain_relay)

        if MitigationType.OBSERVER in selected_mitigations and selected_mitigations[MitigationType.OBSERVER]:
            observer_relay = start_relay()
            logging.info("  Starting socat relay for Observer")
            pt = (observer_relay.stderr.readline().strip().split(b'/')[-1].decode(sys.stdout.encoding),
                  observer_relay.stderr.readline().strip().split(b'/')[-1].decode(sys.stdout.encoding))
            pts.append(pt)
            processes.append(observer_relay)
        else:
            pts.append(None)

        if MitigationType.GATEWAY in selected_mitigations and selected_mitigations[MitigationType.GATEWAY]:
            gateway_relay = start_relay()
            logging.info("  Starting socat relay for Gateway")
            pt = (gateway_relay.stderr.readline().strip().split(b'/')[-1].decode(sys.stdout.encoding),
                  gateway_relay.stderr.readline().strip().split(b'/')[-1].decode(sys.stdout.encoding))
            pts.append(pt)
            processes.append(gateway_relay)
        else:
            pts.append(None)

    except Exception as e:
        logging.exception("Failed to start socat relays. Error: %s", e)
        pts = []
        processes = []
    finally:
        logging.info(f"pts: {pts}")
        return pts, processes


def start_ecus(pts, selected_mitigations: [MitigationType, bool], encryption_key: bytes = None):
    if len(pts) < 3 or len(pts) > 5:
        logging.error("Wrong number of pts!")
        return False

    processes = []

    logging.info("Starting ECUs")
    try:
        logging.info("  Starting Body ECU on pts %s", pts[0][1])
        args = [BASE_PROJECT_PATH + '/penne_ecu/build/bin/penne_ecu', 'body', pts[0][1]]
        if selected_mitigations[MitigationType.ENCRYPTION]:
            args.append(encryption_key)
        process_body = subprocess.Popen(
            args, stdout=subprocess.DEVNULL,
            stderr=subprocess.STDOUT)
        processes.append(process_body)
        logging.info("  Starting Chassis ECU on pts %s", pts[1][1])
        args = [BASE_PROJECT_PATH + '/penne_ecu/build/bin/penne_ecu', 'chassis', pts[1][1]]
        if selected_mitigations[MitigationType.ENCRYPTION]:
            args.append(encryption_key)
        process_chassis = subprocess.Popen(
            args, stdout=subprocess.DEVNULL,
            stderr=subprocess.STDOUT)
        processes.append(process_chassis)
        logging.info("  Starting Powertrain ECU on pts %s", pts[2][1])
        args = [BASE_PROJECT_PATH + '/penne_ecu/build/bin/penne_ecu', 'powertrain', pts[2][1]]
        if selected_mitigations[MitigationType.ENCRYPTION]:
            args.append(encryption_key)
        process_powertrain = subprocess.Popen(
            args, stdout=subprocess.DEVNULL,
            stderr=subprocess.STDOUT)
        processes.append(process_powertrain)

        if MitigationType.OBSERVER in selected_mitigations and selected_mitigations[MitigationType.OBSERVER]:
            assert pts[3] is not None
            logging.info("  Starting Observer ECU on pts %s", pts[3][1])
            args = [BASE_PROJECT_PATH + '/penne_ecu/build/bin/penne_ecu', 'observer', pts[3][1]]
            if selected_mitigations[MitigationType.ENCRYPTION]:
                args.append(encryption_key)
            process_observer = subprocess.Popen(
                args, stdout=subprocess.DEVNULL,
                stderr=subprocess.STDOUT)
            processes.append(process_observer)

        if MitigationType.GATEWAY in selected_mitigations and selected_mitigations[MitigationType.GATEWAY]:
            assert pts[4] is not None
            logging.info("  Starting Gateway ECU on pts %s", pts[4][1])
            args = [BASE_PROJECT_PATH + '/penne_ecu/build/bin/penne_ecu', 'gateway', pts[4][1]]
            if selected_mitigations[MitigationType.ENCRYPTION]:
                args.append(encryption_key)
            process_gateway = subprocess.Popen(
                args, stdout=subprocess.DEVNULL,
                stderr=subprocess.STDOUT)
            processes.append(process_gateway)

    except Exception as e:
        logging.exception("Failed to start ECUs. Error: %s", e)
        return None
    return processes


def main():
    setup_logging("gui.log", True)

    setup_panel = SetupPanel()
    setup_panel.loop()
    if not setup_panel.get_result():
        # finished
        return

    selected_mitigations = setup_panel.selected_mitigations

    logging.info("Starting Atttack Panel")

    args = ["python", BASE_GUI_PATH + '/attack_panel.py']
    if MitigationType.GATEWAY in selected_mitigations and selected_mitigations[MitigationType.GATEWAY]:
        args.append("--use-gateway")
    # TODO check if process dies after main process terminates
    subprocess.Popen(args)

    # TODO change check for len(4) and len(5) according which mitigation type
    pts, relay_processes = start_socat_relays(selected_mitigations)
    if len(pts) < 3 or len(relay_processes) < 3:
        logging.warning("Failed to start socat relays")
        exit(3)

    # Generate a random 256-bit Encryption Key for the AES-GCM Encryption of ECUs CAN messages
    # All ECUs share the same key for now
    encryption_key = None
    if selected_mitigations[MitigationType.ENCRYPTION]:
        encryption_key = generate_256_bit_key()
        logging.info(f"Created Encryption Key: {encryption_key}")

    ecu_processes = start_ecus(pts=pts, selected_mitigations=selected_mitigations, encryption_key=encryption_key)
    # TODO change check for len(4) and len(5) according which mitigation type
    if ecu_processes is None:
        exit(4)

    threads = start_communication_threads(pts, selected_mitigations)

    logging.info("Initializing car")
    car = Car()

    logging.info("Starting GUI")
    gui = GUI(car=car, selected_mitigations=selected_mitigations)

    while True:
        try:
            gui.loop()
        except Exception as e:
            traceback.print_exc()
            logging.error(e)
            break

    print("Quitting GUI")
    gui.quit()
    for thread in threads:
        logging.info(f"Stopping Thread {thread.name}")
        thread.stopped.set()
        thread.thread.join()

    for process in ecu_processes:
        logging.info(f"Killing Process {process.args}")
        process.kill()

    for process in relay_processes:
        logging.info(f"Killing Process {process.args}")
        process.kill()


if __name__ == '__main__':
    main()
