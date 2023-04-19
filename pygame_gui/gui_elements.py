import logging
import time

import numpy as np
import pygame
import pygame.gfxdraw
import os

from constants import get_asset_path
from car import ObserverCode, GatewayCode


class Drawable:
    def __init__(self, path, pos_x, pos_y):
        if path:
            self.image = pygame.image.load(path)
            self.pos_x = pos_x - self.image.get_size()[0] / 2
            self.pos_y = pos_y - self.image.get_size()[1] / 2
        else:
            self.pos_x = pos_x
            self.pos_y = pos_y

    def draw(self, screen):
        screen.blit(self.image, (self.pos_x, self.pos_y))


class Gauge(Drawable):
    def __init__(self, font, pos_x, pos_y, thickness, radius, circle_colour, unit, upper_limit, path=None,
                 low_color=(200, 200, 200), high_color=(200, 200, 200), percent=0):
        super().__init__(path, pos_x, pos_y)
        self.Font = font
        self.x_cord = pos_x
        self.y_cord = pos_y
        self.thickness = thickness
        self.radius = radius
        self.circle_colour = circle_colour
        self.unit = unit
        self.upper_limit = upper_limit
        self.low_color = low_color
        self.high_color = high_color
        self.percent = percent
        self.value = round(self.percent * self.upper_limit / 100)

    def draw(self, screen):
        if self.percent > 100:
            self.percent = 100
        fill_angle = int(self.percent * 270 / 100)
        per = self.percent

        if per <= 40:
            per = 0
        if per > 100:
            per = 100
        if per < 80:
            ac = self.low_color
        else:
            ac = self.high_color

        self.value = round(self.percent * self.upper_limit / 100)
        pertext = self.Font.render(
            str(self.value) + " " + self.unit, 1, ac)
        pertext_rect = pertext.get_rect(
            center=(int(self.x_cord), int(self.y_cord)))
        screen.blit(pertext, pertext_rect)

        for i in range(0, self.thickness):

            pygame.gfxdraw.arc(screen, int(self.x_cord), int(
                self.y_cord), self.radius - i, -225, 270 - 225, self.circle_colour)
            if self.percent > 4:
                pygame.gfxdraw.arc(screen, int(self.x_cord), int(
                    self.y_cord), self.radius - i, -225, fill_angle - 225, ac)

        if self.percent < 4:
            return


class Button(Drawable):
    def __init__(self, path, pos_x, pos_y, callback):
        super().__init__(path, pos_x, pos_y)
        self.rect = self.image.get_rect(
            topleft=(pos_x - self.image.get_size()[0] / 2, pos_y - self.image.get_size()[1] / 2))
        self.callback = callback

    def on_click(self, event):
        if event.button == 1:
            if self.rect.collidepoint(event.pos):
                self.callback()
                self.image.set_alpha(50)

    def on_release(self, event):
        if event.button == 1:
            self.image.set_alpha(255)


class GearText(Drawable):

    def __init__(self, pos_x, pos_y):
        self.gear = 0
        self.font = pygame.font.SysFont('Franklin Gothic Heavy', 40)
        self.pos_x = pos_x
        self.pos_y = pos_y

    def draw(self, screen):
        gear_text = self.font.render("Gear " + str(self.gear), True, (0, 0, 0))
        screen.blit(gear_text, (self.pos_x, self.pos_y))


class Horn:
    def __init__(self):
        self.status = 0
        self.pressed = 0
        self.honking = 0
        buffer = np.sin(2 * np.pi * np.arange(44100) *
                        400 / 44100).astype(np.float32)
        self.sound = pygame.mixer.Sound(buffer)

    def press(self):
        self.pressed = 1

    def release(self):
        self.pressed = 0

    def set_status(self, status):
        if status == 0 and self.status == 1:
            self.sound.stop()
        elif status == 1 and self.status == 0:
            self.sound.play(-1)

        self.status = status


class ParkBrakeButton(Button):
    def __init__(self, path_pulled, path_released, pos_x, pos_y):
        super().__init__(path_released, pos_x, pos_y, self.clicked)
        self.image_pulled = pygame.image.load(path_pulled)
        self.image_released = pygame.image.load(path_released)
        self.rect = self.image.get_rect(
            topleft=(pos_x - self.image.get_size()[0] / 2, pos_y - self.image.get_size()[1] / 2))
        self.pulled = 0
        self.electronic_value = 0

    def clicked(self):
        if self.pulled:
            self.pulled = 0
            self.image = self.image_released
        else:
            self.pulled = 1
            self.image = self.image_pulled


class LightSwitch(Button):

    def __init__(self, path_off, path_low_beam, path_high_beam, pos_x, pos_y):
        super().__init__(path_off, pos_x, pos_y, self.clicked)
        self.image_off = pygame.image.load(path_off)
        self.image_low_beam = pygame.image.load(path_low_beam)
        self.image_high_beam = pygame.image.load(path_high_beam)
        self.state = 0

    def clicked(self):
        if self.state == 0:
            self.state = 1
            self.image = self.image_low_beam
        elif self.state == 1:
            self.state = 2
            self.image = self.image_high_beam
        else:
            self.state = 0
            self.image = self.image_off


class HazardButton(Button):

    def __init__(self, path, pos_x, pos_y):
        super().__init__(path, pos_x, pos_y, self.clicked)
        self.state = 0

    def clicked(self):
        if self.state == 0:
            self.state = 1
            self.image.set_alpha(100)
        elif self.state == 1:
            self.state = 0
            self.image.set_alpha(255)
        else:
            logging.error(f"Hazard Button can't have state {self.state}")


class BlinkerWiperSwitch(Drawable):
    def __init__(self, path_neutral, path_up, path_down, pos_x, pos_y):
        super().__init__(path_neutral, pos_x, pos_y)
        self.state = 1
        self.image_neutral = pygame.image.load(path_neutral)
        self.image_up = pygame.image.load(path_up)
        self.image_down = pygame.image.load(path_down)

    def up(self):
        if self.state < 2:
            self.state += 1
        if self.state == 1:
            self.image = self.image_neutral
        elif self.state == 2:
            self.image = self.image_up

    def down(self):
        if self.state > 0:
            self.state -= 1
        if self.state == 1:
            self.image = self.image_neutral
        elif self.state == 0:
            self.image = self.image_down


class GearSwitch(Drawable):
    def __init__(self, path_p, path_r, path_n, path_d, pos_x, pos_y):
        super().__init__(path_p, pos_x, pos_y)
        self.image_p = pygame.image.load(path_p)
        self.image_r = pygame.image.load(path_r)
        self.image_n = pygame.image.load(path_n)
        self.image_d = pygame.image.load(path_d)
        self.position = 'P'

    def shift_up_button_clicked(self):
        if self.position == 'D':
            self.position = 'N'
            self.image = self.image_n
        elif self.position == 'N':
            self.position = 'R'
            self.image = self.image_r
        elif self.position == 'R':
            self.position = 'P'
            self.image = self.image_p

    def shift_down_button_clicked(self):
        if self.position == 'P':
            self.position = 'R'
            self.image = self.image_r
        elif self.position == 'R':
            self.position = 'N'
            self.image = self.image_n
        elif self.position == 'N':
            self.position = 'D'
            self.image = self.image_d


class GasPedal(Drawable):
    def __init__(self, path, pos_x, pos_y):
        super().__init__(path, pos_x, pos_y)
        self.pressed = 0
        self.percent = 0

    def press(self):
        self.pressed = 1

    def release(self):
        self.pressed = 0


class BrakePedal(Drawable):
    def __init__(self, path, pos_x, pos_y):
        super().__init__(path, pos_x, pos_y)
        self.pressed = 0
        self.percent = 0

    def press(self):
        self.pressed = 1

    def release(self):
        self.pressed = 0


class ECU(Drawable):
    def __init__(self, image_connected, image_disconnected, pos_x, pos_y):
        super().__init__(image_disconnected, pos_x, pos_y)
        self.last_message_timestamp = time.time()
        self.image_connected = pygame.image.load(image_connected)
        self.image_disconnected = pygame.image.load(image_disconnected)

    def draw(self, screen):
        if time.time() - self.last_message_timestamp > 1:
            self.image = self.image_disconnected
        else:
            self.image = self.image_connected
        super().draw(screen)


class Observer(Drawable):
    def __init__(self, image_connected, image_disconnected, image_message, pos_x, pos_y):
        super().__init__(image_disconnected, pos_x, pos_y)
        self.last_message_timestamp = time.time()
        self.image_connected = pygame.image.load(image_connected)
        self.image_disconnected = pygame.image.load(image_disconnected)
        self.image_message = pygame.image.load(image_message)
        self.observer_id = 0
        self.observer_code = ObserverCode.OK
        self.font = pygame.font.SysFont('Franklin Gothic Heavy', 20)
        self.text_id = self.font.render(f"ALL", True, (0, 0, 0))
        self.text_code = self.font.render(f"OK", True, (0, 0, 0))
        self.last_warning_timestamp = 0

    def draw(self, screen):
        # Check for disconnection of the observer ECU
        timestamp = time.time()
        if timestamp - self.last_message_timestamp > 1:
            self.image = self.image_disconnected
            self.text_id = self.font.render(f"", True, (0, 0, 0))
            self.text_code = self.font.render(f"", True, (0, 0, 0))

        else:
            # We want to print the warning for 1 second so we only change the image when the last warning is more then 1 second old
            if timestamp - self.last_warning_timestamp > 1:
                if self.observer_id == 0 and self.observer_code == ObserverCode.OK :
                    self.image = self.image_connected
                    self.text_id = self.font.render(f"ALL", True, (0, 0, 0))
                    self.text_code = self.font.render(f"OK", True, (0, 0, 0))

                else:
                    self.last_warning_timestamp = timestamp
                    self.image = self.image_message
                    self.text_id = self.font.render(
                        f"ID: 0x{self.observer_id:03x}", True, (0, 0, 0))
                    self.text_code = self.font.render(
                        str(self.observer_code).split('.')[1], True, (0, 0, 0))

        # Draw the image before we draw the text otherwise the text would be covered
        super().draw(screen)

        screen.blit(self.text_id, (self.pos_x + 30, self.pos_y + 30))

        screen.blit(self.text_code, (self.pos_x + 30, self.pos_y + 44))


class Gateway(Drawable):
    def __init__(self, image_connected, image_disconnected, image_message, pos_x, pos_y):
        super().__init__(image_disconnected, pos_x, pos_y)
        self.last_message_timestamp = time.time()
        self.image_connected = pygame.image.load(image_connected)
        self.image_disconnected = pygame.image.load(image_disconnected)
        self.image_message = pygame.image.load(image_message)
        self.gateway_id = 0
        self.gateway_code = 0
        self.text = ""
        self.font = pygame.font.SysFont('Franklin Gothic Heavy', 20)

    def draw(self, screen):
        if time.time() - self.last_message_timestamp > 1:
            self.image = self.image_disconnected
            self.text = ""
            text_id = self.font.render(f"", True, (0, 0, 0))
            text_code = self.font.render(f"", True, (0, 0, 0))

        else:
            if self.gateway_code == GatewayCode.OK:
                self.image = self.image_connected
                text_id = self.font.render(f"ALL", True, (0, 0, 0))
                text_code = self.font.render(f"OK", True, (0, 0, 0))

            else:
                self.image = self.image_message
                text_id = self.font.render(
                    f"ID: 0x{self.gateway_id:03x}", True, (0, 0, 0))
                text_code = self.font.render(
                    str(self.gateway_code).split('.')[1], True, (0, 0, 0))

        # Draw the image before we draw the text otherwise the text would be covered
        super().draw(screen)

        screen.blit(text_id, (self.pos_x + 30, self.pos_y + 30))

        screen.blit(text_code, (self.pos_x + 30, self.pos_y + 44))


class SteeringWheel(Drawable):
    def __init__(self, path, pos_x, pos_y):
        super().__init__(path, pos_x, pos_y)
        self.image = pygame.image.load(
            get_asset_path("steering_wheel.svg"))
        self.width, height = self.image.get_size()
        self.physical_angle = 360
        self.electronic_angle = 0

    def draw(self, screen):
        rotated_image = pygame.transform.rotate(
            self.image, self.physical_angle)
        new_rect = rotated_image.get_rect(
            center=self.image.get_rect(topleft=(self.pos_x, self.pos_y)).center)
        screen.blit(rotated_image, new_rect.topleft)


class Indicator(Drawable):
    def __init__(self, path, pos_x, pos_y):
        super().__init__(path, pos_x, pos_y)
        self.shining = 0
        self.image.set_alpha(50)
        self.width, height = self.image.get_size()

    def set_status(self, status):
        if status == 0:
            self.shining = 0
            self.image.set_alpha(50)
        elif status == 1:
            self.shining = 1
            self.image.set_alpha(255)
        else:
            logging.error(f"Indicator received invalid status {status}")


class Engine(Button):
    def __init__(self, path_start, path_stop, pos_x, pos_y):
        super().__init__(path_start, pos_x, pos_y, self.pressed)
        self.status = 0
        self.image_start = pygame.image.load(path_start)
        self.image_stop = pygame.image.load(path_stop)

    def pressed(self):
        self.set_status(0 if self.status else 1)

    def set_status(self, status):
        if status == 0:
            self.status = 0
            self.image = self.image_start
        elif status == 1:
            self.status = 1
            self.image = self.image_stop
        else:
            logging.error("Engine received invalid status %s", status)


class CarLights(Drawable):
    def __init__(self, path_on, path_off, pos_x, pos_y):
        super().__init__(path_off, pos_x, pos_y)
        self.status = 0
        self.image_on = pygame.image.load(path_on)
        self.image_off = pygame.image.load(path_off)

    def set_status(self, status):
        if status == 0:
            self.status = 0
            self.image = self.image_off
        elif status == 1:
            self.status = 1
            self.image = self.image_on


class FrontLights(Drawable):

    def __init__(self, path_off, path_low_beam, path_high_beam, pos_x, pos_y):
        super().__init__(path_off, pos_x, pos_y)
        self.status = 0
        self.image_off = pygame.image.load(path_off)
        self.image_low_beam = pygame.image.load(path_low_beam)
        self.image_high_beam = pygame.image.load(path_high_beam)

    def set_status(self, status):
        if status == 0:
            self.status = 0
            self.image = self.image_off
        elif status == 1:
            self.status = 1
            self.image = self.image_low_beam
        elif status == 2:
            self.status = 2
            self.image = self.image_high_beam


class Wipers(Drawable):
    def __init__(self, path, pos_x, pos_y):
        super().__init__(path, pos_x, pos_y)
        self.status = 0
        self.image.set_alpha(0)

    def set_status(self, status):
        if status == 0:
            self.status = 0
            self.image.set_alpha(0)
        elif status == 1:
            self.status = 1
            self.image.set_alpha(255)
        else:
            logging.error(
                f"Wiper received wrong status : {status}. Must be 0 or 1.")


class FrontWheel(Drawable):
    def __init__(self, path, pos_x, pos_y):
        super().__init__(path, pos_x, pos_y)
        self.angle = 0

    def draw(self, screen):
        rotated_image = pygame.transform.rotate(self.image, self.angle)
        new_rect = rotated_image.get_rect(
            center=self.image.get_rect(topleft=(self.pos_x, self.pos_y)).center)
        screen.blit(rotated_image, new_rect.topleft)


class Lock:
    def __init__(self):
        self.lock_button_state = 0  # 0 => Nothing, 1 => Lock, 2 => Unlock

    def lock_button_press(self):
        self.lock_button_state = 1

    def unlock_button_press(self):
        self.lock_button_state = 2

    def release(self):
        self.lock_button_state = 0


class Door(Drawable):

    def __init__(self, path_door_closed_window_closed, path_door_closed_window_open, path_door_open_window_closed,
                 path_door_open_window_open, pos_x, pos_y):
        super().__init__(path_door_closed_window_closed, pos_x, pos_y)
        self.image_door_closed_window_closed = pygame.image.load(
            path_door_closed_window_closed)
        self.image_door_closed_window_open = pygame.image.load(
            path_door_closed_window_open)
        self.image_door_open_window_closed = pygame.image.load(
            path_door_open_window_closed)
        self.image_door_open_window_open = pygame.image.load(
            path_door_open_window_open)
        self.window_status = 0  # 0 => Window closed, 1 => Window open
        self.door_status = 0  # 0 => Door closed, 1 => Door open
        self.window_button_state = 0  # 0 => Nothing, 1 => Up, 2 => Down
        self.door_button_state = 0  # 0 => Nothing, 1 => Close, 2 => Open

    def window_up_button_press(self):
        self.window_button_state = 1

    def window_down_button_press(self):
        self.window_button_state = 2

    def door_open_button_press(self):
        self.door_button_state = 1

    def door_close_button_press(self):
        self.door_button_state = 2

    def release(self):
        self.window_button_state = 0
        self.door_button_state = 0

    def set_window_status(self, status):
        if status == 0:
            self.window_status = 0
            if self.door_status == 0:
                self.image = self.image_door_closed_window_closed
            elif self.door_status == 1:
                self.image = self.image_door_open_window_closed
            else:
                logging.error(f"Invalid door status {self.door_status}")
        elif status == 1:
            self.window_status = 1
            if self.door_status == 0:
                self.image = self.image_door_closed_window_open
            elif self.door_status == 1:
                self.image = self.image_door_open_window_open
            else:
                logging.error(f"Invalid door status {self.door_status}")
        else:
            logging.error(f"Window received invalid status {status}")

    def set_door_status(self, status):
        if status == 0:
            self.door_status = 0
            if self.window_status == 0:
                self.image = self.image_door_closed_window_closed
            elif self.window_status == 1:
                self.image = self.image_door_closed_window_open
            else:
                logging.error(f"Invalid window status {self.window_status}")
        elif status == 1:
            self.door_status = 1
            if self.window_status == 0:
                self.image = self.image_door_open_window_closed
            elif self.window_status == 1:
                self.image = self.image_door_open_window_open
            else:
                logging.error(f"Invalid window status {self.window_status}")
        else:
            logging.error(f"Door received invalid status {status}")
