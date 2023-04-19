import traceback

import pygame.gfxdraw
from gui_elements import *
from pubsub import pub

from constants import get_asset_path
from simulator import simulate
import time
from car import *
from setup_panel import MitigationType

# Create custom events that can be used for callbacks when a message is received from the ECUs
POWERTRAIN_RECEIVED = pygame.USEREVENT + 1
CHASSIS_RECEIVED = pygame.USEREVENT + 2
BODY_RECEIVED = pygame.USEREVENT + 3
OBSERVER_RECEIVED = pygame.USEREVENT + 4
GATEWAY_RECEIVED = pygame.USEREVENT + 5


class GUI:
    def __init__(self, car: Car,
                 selected_mitigations: [MitigationType, bool] = {mitigationType: False for mitigationType in
                                                                 MitigationType}):
        self.car = car

        # make sure the dictionary has a valid state
        selected_mitigations = {key: selected_mitigations[key]
                                if key in selected_mitigations else
                                False for key in MitigationType}

        self.selected_mitigations = selected_mitigations
        self.bg_c = (100, 100, 100)
        self.circle_c = (55, 77, 91)

        pygame.init()
        self.width, self.height = (1200, 800)
        self.clock = pygame.time.Clock()
        self.fps = 20
        self.screen = pygame.display.set_mode((self.width, self.height))
        pygame.mixer.init(size=32)

        self.elements = {}
        self.add_gui_elements()

        pygame.display.set_caption('PENNE Dashboard')

    def quit(self):
        pygame.quit()

    def add_gui_elements(self):
        self.elements["quit_button"] = Button(get_asset_path("quit_button.svg"),
                                              self.width * 0.98,
                                              self.height * 0.03,
                                              self.quit)
        self.elements["steering_wheel"] = SteeringWheel(get_asset_path("steering_wheel.svg"),
                                                        self.width * 0.417,
                                                        self.height * 0.65)

        # Switches left and right to the steering wheel
        self.elements["blinker_switch"] = BlinkerWiperSwitch(get_asset_path("blinker_switch.svg"),
                                                             get_asset_path("blinker_switch_up.svg"),
                                                             get_asset_path("blinker_switch_down.svg"),
                                                             self.width * 0.2625,
                                                             self.height * 0.65)
        self.elements["wiper_switch"] = BlinkerWiperSwitch(get_asset_path("wiper_switch.svg"),
                                                           get_asset_path("wiper_switch_up.svg"),
                                                           get_asset_path("wiper_switch_down.svg"),
                                                           self.width * 0.567,
                                                           self.height * 0.65)

        self.elements["gear_switch"] = GearSwitch(path_p=get_asset_path("shift_lever_p.svg"),
                                                  path_r=get_asset_path("shift_lever_r.svg"),
                                                  path_n=get_asset_path("shift_lever_n.svg"),
                                                  path_d=get_asset_path("shift_lever_d.svg"),
                                                  pos_x=self.width * 0.69,
                                                  pos_y=self.height * 0.7)

        # Pedals at the bottom
        self.elements["gas_pedal"] = GasPedal(get_asset_path("gas_pedal.svg"), self.width * 0.47,
                                              self.height * 0.89)
        self.elements["brake_pedal"] = BrakePedal(get_asset_path("brake_pedal.svg"),
                                                  self.width * 0.38,
                                                  self.height * 0.9)

        # Indicators
        self.elements["blinker_indicator_left"] = Indicator(
            get_asset_path("turn_signal_left.svg"), self.width * 0.37, self.height * 0.08)
        self.elements["blinker_indicator_right"] = Indicator(
            get_asset_path("turn_signal_right.svg"), self.width * 0.45, self.height * 0.08)
        self.elements["check_engine_indicator"] = Indicator(
            get_asset_path("check_engine.svg"), self.width * 0.41, self.height * 0.08)
        self.elements["park_brake_indicator"] = Indicator(
            get_asset_path("park_brake.png"), self.width * 0.37, self.height * 0.15)
        self.elements["door_lock_indicator"] = Indicator(
            get_asset_path("door_lock_indicator.svg"), self.width * 0.41, self.height * 0.15)
        self.elements["door_open_indicator"] = Indicator(
            get_asset_path("door_open_indicator.svg"), self.width * 0.45, self.height * 0.15)

        self.elements["park_brake"] = ParkBrakeButton(get_asset_path("park_brake_pulled.svg"),
                                                      get_asset_path("park_brake_released.svg"),
                                                      self.width * 0.708, self.height * 0.91)

        self.elements["shift_up_button"] = Button(
            get_asset_path("arrow_up.svg"), self.width *
            0.72, self.height * 0.66,
            self.elements["gear_switch"].shift_up_button_clicked)

        self.elements["shift_down_button"] = Button(
            get_asset_path("arrow_down.svg"), self.width *
            0.72, self.height * 0.75,
            self.elements["gear_switch"].shift_down_button_clicked)

        self.elements["blinker_up_button"] = Button(
            get_asset_path("arrow_up.svg"), self.width *
            0.25, self.height * 0.595,
            self.elements["blinker_switch"].up)

        self.elements["blinker_down_button"] = Button(
            get_asset_path("arrow_down.svg"), self.width *
            0.25, self.height * 0.705,
            self.elements["blinker_switch"].down)

        self.elements["wiper_up_button"] = Button(
            get_asset_path("arrow_up.svg"), self.width *
            0.583, self.height * 0.595,
            self.elements["wiper_switch"].up)

        self.elements["wiper_down_button"] = Button(
            get_asset_path("arrow_down.svg"), self.width *
            0.583, self.height * 0.705,
            self.elements["wiper_switch"].down)

        self.elements["horn"] = Horn()

        self.elements["horn_button"] = Button(
            get_asset_path("horn.svg"), self.width * 0.417, self.height * 0.65,
            self.elements["horn"].press)

        # ECUs
        self.elements["body_ecu"] = ECU(get_asset_path("body_ecu_green.svg"),
                                        get_asset_path("body_ecu_red.svg"),
                                        self.width * 0.1, self.height * 0.42)
        self.elements["chassis_ecu"] = ECU(get_asset_path("chassis_ecu_green.svg"),
                                           get_asset_path("chassis_ecu_red.svg"),
                                           self.width * 0.1,
                                           self.height * 0.54)
        self.elements["powertrain_ecu"] = ECU(get_asset_path("powertrain_ecu_green.svg"),
                                              get_asset_path("powertrain_ecu_red.svg"),
                                              self.width * 0.1,
                                              self.height * 0.66)

        self.elements["observer_ecu"] = Observer(
            image_connected=get_asset_path("observer_ecu_green.svg"),
            image_disconnected=get_asset_path("observer_ecu_red.svg"),
            image_message=get_asset_path("observer_ecu_orange.svg"),
            pos_x=self.width * 0.1,
            pos_y=self.height * 0.78)

        self.elements["gateway_ecu"] = Gateway(
            image_connected=get_asset_path("gateway_ecu_green.svg"),
            image_disconnected=get_asset_path("gateway_ecu_red.svg"),
            image_message=get_asset_path("gateway_ecu_orange.svg"),
            pos_x=self.width * 0.1,
            pos_y=self.height * 0.9)

        can_bus_image = get_asset_path("can_bus_gateway_on.svg"
                                       if self.selected_mitigations[MitigationType.GATEWAY] else
                                       "can_bus_gateway_off.svg")

        self.elements["can_bus"] = Drawable(path=can_bus_image,
                                            pos_x=self.width * 0.13,
                                            pos_y=self.height * 0.695)

        if self.selected_mitigations[MitigationType.ENCRYPTION]:
            self.elements["ecus_encrypted"] = Drawable(path=get_asset_path("ecus_encrypted.svg"),
                                                       pos_x=self.width * 0.103,
                                                       pos_y=self.height * 0.659)

        self.elements["engine"] = Engine(get_asset_path("engine_start.svg"),
                                         get_asset_path("engine_stop.svg"),
                                         self.width * 0.517,
                                         self.height * 0.77)

        large_font = pygame.font.SysFont('Franklin Gothic Heavy', 60)

        self.elements["kph_gauge"] = Gauge(
            font=large_font,
            pos_x=self.width * 0.2,
            pos_y=self.height * 0.2,
            thickness=30,
            radius=150,
            circle_colour=self.circle_c,
            unit="kmh",
            upper_limit=250,
            percent=0)

        self.elements["rpm_gauge"] = Gauge(
            font=large_font,
            pos_x=self.width * 0.633,
            pos_y=self.height * 0.2,
            thickness=30,
            radius=150,
            circle_colour=self.circle_c,
            unit="rpm",
            upper_limit=8000,
            low_color=(0, 255, 0),
            high_color=(255, 0, 0),
            percent=0)

        self.elements["gear_text"] = GearText(
            pos_x=self.width * 0.6, pos_y=self.height * 0.3)

        self.elements["car_frontlights"] = FrontLights(path_off=get_asset_path("front_lights_off.svg"),
                                                       path_low_beam=get_asset_path("front_lights_low_beam.svg"),
                                                       path_high_beam=get_asset_path("front_lights_high_beam.svg"),
                                                       pos_x=self.width * 0.897,
                                                       pos_y=self.height * 0.4708)

        self.elements["car_rearlights"] = CarLights(path_on=get_asset_path("rear_lights_on.svg"),
                                                    path_off=get_asset_path("rear_lights_off.svg"),
                                                    pos_x=self.width * 0.902,
                                                    pos_y=self.height * 0.929)

        self.elements["car_brakelights"] = CarLights(path_on=get_asset_path("brake_lights_on.svg"),
                                                     path_off=get_asset_path("brake_lights_off.svg"),
                                                     pos_x=self.width * 0.9025,
                                                     pos_y=self.height * 0.925)

        self.elements["car_front_wipers"] = Wipers(path=get_asset_path("front_wipers.svg"),
                                                   pos_x=self.width * 0.901,
                                                   pos_y=self.height * 0.638)

        self.elements["car_rear_wipers"] = Wipers(path=get_asset_path("rear_wipers.svg"),
                                                  pos_x=self.width * 0.901,
                                                  pos_y=self.height * 0.844)

        self.elements["car_blinkers_left"] = CarLights(
            path_on=get_asset_path("left_blinkers_on.svg"),
            path_off=get_asset_path("left_blinkers_off.svg"),
            pos_x=self.width * 0.853,
            pos_y=self.height * 0.7138)

        self.elements["car_blinkers_right"] = CarLights(
            path_on=get_asset_path("right_blinkers_on.svg"),
            path_off=get_asset_path("right_blinkers_off.svg"),
            pos_x=self.width * 0.9439,
            pos_y=self.height * 0.713)

        self.elements["rear_wheel_left"] = Drawable(path=get_asset_path("wheel.svg"),
                                                    pos_x=self.width * 0.84, pos_y=self.height * 0.85)

        self.elements["rear_wheel_right"] = Drawable(path=get_asset_path("wheel.svg"),
                                                     pos_x=self.width * 0.962, pos_y=self.height * 0.85)

        self.elements["front_wheel_left"] = FrontWheel(path=get_asset_path("wheel.svg"),
                                                       pos_x=self.width * 0.84, pos_y=self.height * 0.58)
        self.elements["front_wheel_right"] = FrontWheel(path=get_asset_path("wheel.svg"),
                                                        pos_x=self.width * 0.957, pos_y=self.height * 0.58)

        self.elements["car_chassis"] = Drawable(path=get_asset_path("car_chassis.svg"),
                                                pos_x=self.width * 0.9,
                                                pos_y=self.height * 0.7)

        self.elements["light_switch"] = LightSwitch(path_off=get_asset_path("light_switch_off.svg"),
                                                    path_low_beam=get_asset_path("light_switch_low_beam.svg"),
                                                    path_high_beam=get_asset_path("light_switch_high_beam.svg"),
                                                    pos_x=self.width * 0.27, pos_y=self.height * 0.85)

        self.elements["hazard_button"] = HazardButton(path=get_asset_path("hazard_button.svg"),
                                                      pos_x=self.width * 0.58, pos_y=self.height * 0.77)

        self.elements["left_door"] = Door(
            path_door_closed_window_closed=get_asset_path("left_door_closed_window_closed.svg"),
            path_door_closed_window_open=get_asset_path("left_door_closed_window_open.svg"),
            path_door_open_window_closed=get_asset_path("left_door_open_window_closed.svg"),
            path_door_open_window_open=get_asset_path("left_door_open_window_open.svg"),
            pos_x=self.width * 0.8245, pos_y=self.height * 0.698)
        self.elements["right_door"] = Door(
            path_door_closed_window_closed=get_asset_path("right_door_closed_window_closed.svg"),
            path_door_closed_window_open=get_asset_path("right_door_closed_window_open.svg"),
            path_door_open_window_closed=get_asset_path("right_door_open_window_closed.svg"),
            path_door_open_window_open=get_asset_path("right_door_open_window_open.svg"),
            pos_x=self.width * 0.9752, pos_y=self.height * 0.702)

        self.elements["left_window_switch"] = Drawable(path=get_asset_path("left_window_switch.svg"),
                                                       pos_x=self.width * 0.68, pos_y=self.height * 0.47)

        self.elements["left_window_up_button"] = Button(path=get_asset_path("arrow_up.svg"),
                                                        pos_x=self.width * 0.68, pos_y=self.height * 0.43,
                                                        callback=self.elements["left_door"].window_up_button_press)
        self.elements["left_window_down_button"] = Button(path=get_asset_path("arrow_down.svg"),
                                                          pos_x=self.width * 0.68, pos_y=self.height * 0.51,
                                                          callback=self.elements[
                                                              "left_door"].window_down_button_press)

        self.elements["right_window_switch"] = Drawable(path=get_asset_path("right_window_switch.svg"),
                                                        pos_x=self.width * 0.73, pos_y=self.height * 0.47)
        self.elements["right_window_up_button"] = Button(path=get_asset_path("arrow_up.svg"),
                                                         pos_x=self.width * 0.73, pos_y=self.height * 0.43,
                                                         callback=self.elements["right_door"].window_up_button_press)
        self.elements["right_window_down_button"] = Button(path=get_asset_path("arrow_down.svg"),
                                                           pos_x=self.width * 0.73, pos_y=self.height * 0.51,
                                                           callback=self.elements[
                                                               "right_door"].window_down_button_press)

        self.elements["lock"] = Lock()

        self.elements["door_lock_button"] = Button(path=get_asset_path("door_lock_button.svg"),
                                                   pos_x=self.width * 0.56, pos_y=self.height * 0.5,
                                                   callback=self.elements["lock"].lock_button_press)

        self.elements["door_unlock_button"] = Button(path=get_asset_path("door_unlock_button.svg"),
                                                     pos_x=self.width * 0.56, pos_y=self.height * 0.44,
                                                     callback=self.elements["lock"].unlock_button_press)

        self.elements["open_left_door_button"] = Button(
            path=get_asset_path("/open_left_door_button.svg"),
            pos_x=self.width * 0.6, pos_y=self.height * 0.44,
            callback=self.elements["left_door"].door_open_button_press)

        self.elements["close_left_door_button"] = Button(
            path=get_asset_path("close_left_door_button.svg"),
            pos_x=self.width * 0.6, pos_y=self.height * 0.5,
            callback=self.elements["left_door"].door_close_button_press)

        self.elements["open_right_door_button"] = Button(
            path=get_asset_path("open_right_door_button.svg"),
            pos_x=self.width * 0.64, pos_y=self.height * 0.44,
            callback=self.elements["right_door"].door_open_button_press)

        self.elements["close_right_door_button"] = Button(
            path=get_asset_path("close_right_door_button.svg"),
            pos_x=self.width * 0.64, pos_y=self.height * 0.5,
            callback=self.elements["right_door"].door_close_button_press)

    def send_serial_update(self):
        # Chassis
        msg = f" 00{self.car.brake_value:X}" \
              f" 01{self.car.accelerator_value:X}" \
              f" 02{self.car.steering_value:X}" \
              f" 03{ord(self.car.shift_value):X}" \
              f" 04{self.car.turn_switch_value:X}" \
              f" 05{self.car.horn_value:X}" \
              f" 06{self.car.light_switch_value:X}" \
              f" 07{self.car.light_flash_value:X}" \
              f" 08{self.car.parking_value:X}" \
              f" 09{self.car.wiper_f_sw_value:X}" \
              f" 0A{self.car.wiper_r_sw_value:X}" \
              f" 0B{self.car.door_lock_value:X}" \
              f" 0C{self.car.l_door_handle_value:X}" \
              f" 0D{self.car.r_door_handle_value:X}" \
              f" 0E{self.car.l_window_switch_value:X}" \
              f" 0F{self.car.r_window_switch_value:X}" \
              f" 10{self.car.hazard_value:X}" \
              f" 11{self.car.engine_value:X}"
        pub.sendMessage("chassis", msg=msg)

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
                self.elements["horn"].release()
                self.elements["left_door"].release()
                self.elements["right_door"].release()
                self.elements["lock"].release()

            if event.type == POWERTRAIN_RECEIVED:
                self.parse_and_update_powertrain_data(event.message)
                self.elements["powertrain_ecu"].last_message_timestamp = time.time()

            if event.type == CHASSIS_RECEIVED:
                self.parse_and_update_chassis_data(event.message)
                self.elements["chassis_ecu"].last_message_timestamp = time.time()

            if event.type == BODY_RECEIVED:
                self.parse_and_update_body_data(event.message)
                self.elements["body_ecu"].last_message_timestamp = time.time()

            if event.type == OBSERVER_RECEIVED:
                self.parse_and_update_observer_data(event.message)
                self.elements["observer_ecu"].last_message_timestamp = time.time()

            if event.type == GATEWAY_RECEIVED:
                self.parse_and_update_gateway_data(event.message)
                self.elements["gateway_ecu"].last_message_timestamp = time.time()

        keys = pygame.key.get_pressed()
        if keys[pygame.K_LEFT]:
            if self.elements["steering_wheel"].physical_angle < 720:
                self.elements["steering_wheel"].physical_angle += 10
        if keys[pygame.K_RIGHT]:
            if self.elements["steering_wheel"].physical_angle > 0:
                self.elements["steering_wheel"].physical_angle -= 10
        if not keys[pygame.K_LEFT] and not keys[pygame.K_RIGHT]:
            if self.elements["steering_wheel"].physical_angle > 360:
                self.elements["steering_wheel"].physical_angle -= 10
            elif self.elements["steering_wheel"].physical_angle < 360:
                self.elements["steering_wheel"].physical_angle += 10
        if keys[pygame.K_UP]:
            self.elements["gas_pedal"].press()
        else:
            self.elements["gas_pedal"].release()
        if keys[pygame.K_DOWN]:
            self.elements["brake_pedal"].press()
        else:
            self.elements["brake_pedal"].release()

        simulate(self.car, self.elements)
        self.update_gui_elements_from_car_values()

        self.screen.fill(self.bg_c)

        for name, element in self.elements.items():
            if isinstance(element, Drawable):
                element.draw(self.screen)

        # Send the status of all GUI elements to the ECUs
        self.send_serial_update()

        pygame.display.update()
        self.clock.tick(self.fps)

    def update_gui_elements_from_car_values(self):

        self.elements["front_wheel_left"].angle = self.car.power_steering
        self.elements["front_wheel_right"].angle = self.car.power_steering
        self.elements["engine"].set_status(self.car.engine_value)
        self.elements["park_brake_indicator"].set_status(
            self.car.parking_brake_status)
        self.elements["rpm_gauge"].percent = int(
            self.car.engine_rpm / 65535 * 100)
        self.elements["kph_gauge"].percent = int(
            self.car.speed_kph / 255 * 100)
        self.elements["gear_text"].gear = self.car.gear
        self.elements["car_frontlights"].set_status(self.car.light_status)
        self.elements["car_rearlights"].set_status(self.car.light_status)
        self.elements["horn"].set_status(self.car.horn_operation)
        self.elements["car_brakelights"].set_status(self.car.brake_output > 0)
        millis = int(time.time() * 1000) % 1000
        self.elements["blinker_indicator_left"].set_status(
            (self.car.turn_signal_indicator & 0xFF ==
             0 or self.car.turn_signal_indicator == 3)
            and millis > 500)
        self.elements["blinker_indicator_right"].set_status(
            (self.car.turn_signal_indicator & 0xFF ==
             2 or self.car.turn_signal_indicator == 3)
            and millis > 500)
        self.elements["car_blinkers_left"].set_status(
            (self.car.turn_signal_indicator ==
             0 or self.car.turn_signal_indicator == 3)
            and millis > 500)
        self.elements["car_blinkers_right"].set_status(
            (self.car.turn_signal_indicator ==
             2 or self.car.turn_signal_indicator == 3)
            and millis > 500)
        self.elements["check_engine_indicator"].set_status(
            self.car.engine_status == 0)  # Light this lamp before engine is started
        self.elements["door_lock_indicator"].set_status(
            self.car.door_lock_indicator)
        self.elements["door_open_indicator"].set_status(
            self.car.door_open_indicator)
        self.elements["car_front_wipers"].set_status(
            self.car.front_wiper_status)
        self.elements["car_rear_wipers"].set_status(self.car.rear_wiper_status)
        self.elements["left_door"].set_window_status(
            self.car.l_window_position)
        self.elements["right_door"].set_window_status(
            self.car.r_window_position)
        self.elements["left_door"].set_door_status(self.car.l_door_position)
        self.elements["right_door"].set_door_status(self.car.r_door_position)
        self.elements["observer_ecu"].observer_id = self.car.observer_id
        self.elements["observer_ecu"].observer_code = self.car.observer_code
        self.elements["gateway_ecu"].gateway_id = self.car.gateway_id
        self.elements["gateway_ecu"].gateway_code = self.car.gateway_code

    def parse_and_update_powertrain_data(self, message: str):
        if message[0:4] == "EXU ":
            message = message.replace("EXU", "").strip()
            parts = message.split(" ")
            for part in parts:
                if part[0:2] == "00":
                    self.car.shift_position = int(part[2:], 16)
                if part[0:2] == "01":
                    self.car.engine_status = int(part[2:], 16)
                if part[0:2] == "02":
                    self.car.brake_output = int(part[2:], 16)
                if part[0:2] == "03":
                    self.car.parking_brake_status = int(part[2:], 16)
                if part[0:2] == "04":
                    self.car.gear = int(part[2:], 16)
                if part[0:2] == "05":
                    self.car.power_steering = int(part[2:], 16)

    def parse_and_update_chassis_data(self, message: str):
        if message[0:4] == "EXU ":
            message = message.replace("EXU", "").strip()
            parts = message.split(" ")
            for part in parts:
                if part[0:2] == "00":
                    self.car.engine_rpm = int(part[2:], 16)
                if part[0:2] == "01":
                    self.car.shift_position = int(part[2:], 16)
                if part[0:2] == "02":
                    self.car.engine_status = int(part[2:], 16)
                if part[0:2] == "03":
                    self.car.parking_brake_status = int(part[2:], 16)
                if part[0:2] == "04":
                    self.car.turn_signal_indicator = int(part[2:], 16)
                if part[0:2] == "05":
                    self.car.door_open_indicator = int(part[2:], 16)
                if part[0:2] == "06":
                    self.car.door_lock_indicator = int(part[2:], 16)
                if part[0:2] == "07":
                    self.car.speed_kph = int(part[2:], 16)

    def parse_and_update_body_data(self, message: str):
        if message[0:4] == "EXU ":
            message = message.replace("EXU", "").strip()
            parts = message.split(" ")
            for part in parts:
                if part[0:2] == "00":
                    self.car.horn_operation = int(part[2:], 16)
                if part[0:2] == "01":
                    self.car.light_status = int(part[2:], 16)
                if part[0:2] == "02":
                    self.car.turn_signal_indicator = int(part[2:], 16)
                if part[0:2] == "03":
                    self.car.front_wiper_status = int(part[2:], 16)
                if part[0:2] == "04":
                    self.car.rear_wiper_status = int(part[2:], 16)
                if part[0:2] == "05":
                    self.car.door_lock_status = int(part[2:], 16)
                if part[0:2] == "06":
                    self.car.l_door_position = int(part[2:], 16)
                if part[0:2] == "07":
                    self.car.r_door_position = int(part[2:], 16)
                if part[0:2] == "08":
                    self.car.l_window_position = int(part[2:], 16)
                if part[0:2] == "09":
                    self.car.r_window_position = int(part[2:], 16)

    def parse_and_update_observer_data(self, message: str):
        if message[0:4] == "EXU ":
            message = message.replace("EXU", "").strip()
            parts = message.split(" ")
            for part in parts:
                if part[0:2] == "00":
                    self.car.observer_id = int(part[2:], 16)
                if part[0:2] == "01":
                    self.car.observer_code = ObserverCode(int(part[2:], 16))

    def parse_and_update_gateway_data(self, message: str):
        if message[0:4] == "EXU ":
            message = message.replace("EXU", "").strip()
            parts = message.split(" ")
            for part in parts:
                if part[0:2] == "00":
                    self.car.gateway_id = int(part[2:], 16)
                if part[0:2] == "01":
                    self.car.gateway_code = GatewayCode(int(part[2:], 16))


def main():
    car = Car()
    selected_mitigations = {mitigationType: False for mitigationType in MitigationType}
    gui = GUI(car=car, selected_mitigations=selected_mitigations)

    while True:
        try:
            gui.loop()
        except Exception as e:
            traceback.print_exc()
            logging.error(e)
            break


if __name__ == '__main__':
    main()
