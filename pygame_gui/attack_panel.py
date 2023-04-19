import argparse
import logging
import threading
import attacks

from attack_panel_elements import *
from constants import get_asset_path


class AttackPanel:
    def __init__(self, using_gateway=False) -> None:
        self.bg_c = (100, 100, 100)
        self.width, self.height = (600, 600)
        self.fps = 20

        pygame.init()
        self.clock = pygame.time.Clock()
        self.screen = pygame.display.set_mode((self.width, self.height))
        pygame.mixer.init(size=32)
        pygame.display.set_caption('Attack Panel')

        self.elements = {}
        self.add_gui_elements()
        self.running = True

        self.using_gateway = using_gateway
        print(using_gateway)
        print(self.using_gateway)
        self.using_obd2 = True

    def quit(self):
        self.running = False
        pygame.quit()

    def toggle_obd2_clipping_switch(self):
        if not self.using_obd2:
            self.using_obd2 = True
            self.elements["obd_clipping_toggle"].toggle()
        elif self.using_obd2:
            self.using_obd2 = False
            self.elements["obd_clipping_toggle"].toggle()

    def add_gui_elements(self):
        self.elements["skull"] = Drawable(path=get_asset_path("skull.png"),
                                          pos_x=self.width * 0.5,
                                          pos_y=self.height * 0.12)

        self.elements["obd_clipping_toggle"] = ToggleButton(path_a=get_asset_path("obd2_selected.svg"),
                                                            path_b=get_asset_path("clipping_selected.svg"),
                                                            pos_x=self.width * 0.5,
                                                            pos_y=self.height * 0.3,
                                                            callback=self.toggle_obd2_clipping_switch)

        self.elements["quit_button"] = Button(path=get_asset_path("quit_button.svg"),
                                              pos_x=self.width * 0.95,
                                              pos_y=self.height * 0.05,
                                              callback=self.quit)

        self.elements["engine_off_attack_button"] = Button(
            path=get_asset_path("engine_off_attack_button.svg"),
            pos_x=self.width * 0.25,
            pos_y=self.height * 0.42,
            callback=self.engine_off_attack)

        self.elements["unlock_attack_button"] = Button(
            path=get_asset_path("unlock_attack_button.svg"),
            pos_x=self.width * 0.75,
            pos_y=self.height * 0.42,
            callback=self.unlock_attack)

        self.elements["rpm_attack_button"] = Button(path=get_asset_path("rpm_attack_button.svg"),
                                                    pos_x=self.width * 0.25,
                                                    pos_y=self.height * 0.54,
                                                    callback=self.rpm_attack)
        self.elements["brake_attack_button"] = Button(path=get_asset_path("brake_attack_button.svg"),
                                                      pos_x=self.width * 0.75,
                                                      pos_y=self.height * 0.54,
                                                      callback=self.brake_attack)
        self.elements["horn_attack_button"] = Button(
            path=get_asset_path("horn_attack_button.svg"),
            pos_x=self.width * 0.25,
            pos_y=self.height * 0.66,
            callback=self.horn_attack)
        self.elements["random_attack_button"] = Button(
            path=get_asset_path("random_attack_button.svg"),
            pos_x=self.width * 0.75,
            pos_y=self.height * 0.66,
            callback=self.random_attack)

        self.elements["gas_pedal_attack_button"] = Button(
            path=get_asset_path("gas_pedal_attack_button.svg"),
            pos_x=self.width * 0.25,
            pos_y=self.height * 0.78,
            callback=self.gas_pedal_attack)

        self.elements["fuzz_attack_button"] = Button(
            path=get_asset_path("fuzzing_attack_button.svg"),
            pos_x=self.width * 0.75,
            pos_y=self.height * 0.78,
            callback=self.fuzzing_attack)

        self.elements["sniff_attack_button"] = Button(
            path=get_asset_path("sniffing_attack_button.svg"),
            pos_x=self.width * 0.5,
            pos_y=self.height * 0.9,
            callback=self.sniff_attack)

    def rpm_attack(self):
        # Starts CAN RPM attack in a new thread
        logging.info("Starting rpm attack")
        attack_thread = threading.Thread(target=attacks.rpm_attack, args=[self.using_gateway, self.using_obd2])
        attack_thread.start()

    def brake_attack(self):
        logging.info("Starting brake attack")
        attack_thread = threading.Thread(target=attacks.brake_attack, args=[self.using_gateway, self.using_obd2])
        attack_thread.start()

    def engine_off_attack(self):
        logging.info("Starting engine_off attack")
        attack_thread = threading.Thread(target=attacks.engine_off_attack, args=[self.using_gateway, self.using_obd2])
        attack_thread.start()

    def reverse_steering_attack(self):
        logging.info("Starting reverse steering attack")
        attack_thread = threading.Thread(target=attacks.reverse_steering_attack,
                                         args=[self.using_gateway, self.using_obd2])
        attack_thread.start()

    def sniff_attack(self):
        logging.info("Starting sniff attack")
        attack_thread = threading.Thread(target=attacks.sniff_attack, args=[self.using_gateway, self.using_obd2])
        attack_thread.start()

    def horn_attack(self):
        logging.info("Starting horn attack")
        attack_thread = threading.Thread(target=attacks.horn_attack, args=[self.using_gateway, self.using_obd2])
        attack_thread.start()

    def random_attack(self):
        logging.info("Starting random attack")
        attack_thread = threading.Thread(target=attacks.random_attack, args=[self.using_gateway, self.using_obd2])
        attack_thread.start()

    def unlock_attack(self):
        logging.info("Starting unlock attack")
        attack_thread = threading.Thread(target=attacks.unlock_attack, args=[self.using_gateway, self.using_obd2])
        attack_thread.start()

    def gas_pedal_attack(self):
        logging.info("Starting gas pedal attack")
        attack_thread = threading.Thread(target=attacks.gas_pedal_attack, args=[self.using_gateway, self.using_obd2])
        attack_thread.start()

    def fuzzing_attack(self):
        logging.info("Starting fuzzing attack")
        attack_thread = threading.Thread(target=attacks.fuzzing_attack, args=[self.using_gateway, self.using_obd2])
        attack_thread.start()

    def loop(self):
        events = pygame.event.get()
        for event in events:

            if event.type == pygame.MOUSEBUTTONDOWN:
                for name, element in self.elements.items():
                    if isinstance(element, Button):
                        element.on_click(event)
            if event.type == pygame.MOUSEBUTTONUP:
                for name, element in self.elements.items():
                    if isinstance(element, Button):
                        element.on_release(event)

        self.screen.fill(self.bg_c)

        for name, element in self.elements.items():
            if isinstance(element, Drawable):
                element.draw(self.screen)

        pygame.display.update()
        self.clock.tick(self.fps)


if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        prog='Attack Panel',
        description='Tool for using predefined attacks attacks for PENNE')

    parser.add_argument(
        '--use-gateway',  action='store_true',
        help='Indicates if a gateway is used. If yes the vcan1 is used for ODB2 otherwise vcan0')

    args = parser.parse_args()

    attack_panel = AttackPanel(using_gateway=args.use_gateway)

    if attack_panel is None:
        logging.error("Failed to initialize attack_panel")
        exit(2)
    while attack_panel.running:
        try:
            attack_panel.loop()
        except Exception as e:
            break
