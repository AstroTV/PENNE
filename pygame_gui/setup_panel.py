import logging
import pygame
import traceback

from enum import Enum
from constants import *
from base_views import Drawable, TextField, Button
from setup_panel_views import MitigationView


class MitigationType(Enum):
    OBSERVER = 1
    GATEWAY = 2
    ENCRYPTION = 3


class SetupPanel:
    def __init__(self) -> None:

        self.bg_c = COLOR_MAIN_BACKGROUND
        self.width, self.height = (400, 400)
        self.fps = 20

        pygame.init()
        self.clock = pygame.time.Clock()
        self.screen = pygame.display.set_mode((self.width, self.height))
        pygame.mixer.init(size=32)
        pygame.display.set_caption('Setup Panel')

        self.views = []
        self.views_mitigation = []
        self.add_gui_elements()
        self.result = False
        self.selected_mitigations = {mitigationType: False for mitigationType in MitigationType}

        self.running = True

    def add_gui_elements(self):
        self.views.append(TextField(40, 20, SETUP_PANEL_TITLE, SETUP_TITLE_COLOR, SETUP_TITLE_FONT,
                                    SETUP_TITLE_FONT_SIZE))

        start_value_y = 100
        mitigation_view_monitoring = MitigationView(40, start_value_y, get_asset_path("setup_monitoring.svg"),
                                                    SETUP_MONITORING_TITLE,
                                                    SETUP_MONITORING_DESCRIPTION_SHORT,
                                                    self.screen, self.select_mitigation,
                                                    MitigationType.OBSERVER)
        self.views.append(mitigation_view_monitoring)
        self.views_mitigation.append(mitigation_view_monitoring)

        start_value_y += 100

        mitigation_view_encryption = MitigationView(40, start_value_y, get_asset_path("setup_encrypted.svg"),
                                                    SETUP_ENCRYPTION_TITLE,
                                                    SETUP_ENCRYPTION_DESCRIPTION_SHORT,
                                                    self.screen, self.select_mitigation,
                                                    MitigationType.ENCRYPTION)
        self.views.append(mitigation_view_encryption)
        self.views_mitigation.append(mitigation_view_encryption)

        start_value_y += 100

        mitigation_view_encryption_gateway = MitigationView(40, start_value_y, get_asset_path("setup_gateway.svg"),
                                                            SETUP_GATEWAY_TITLE, SETUP_GATEWAY_DESCRIPTION_SHORT,
                                                            self.screen, self.select_mitigation,
                                                            MitigationType.GATEWAY)
        self.views.append(mitigation_view_encryption_gateway)
        self.views_mitigation.append(mitigation_view_encryption_gateway)

        self.views.append(Button(get_asset_path("setup_start_symbol.svg"), self.width - 50,
                                 self.height - 50, self.on_ok))

    def get_result(self):
        return self.result

    def on_ok(self):
        self.quit(True)

    def quit(self, result):
        self.result = result
        self.running = False
        pygame.quit()

    def select_mitigation(self, mitigation_type: MitigationType):
        self.selected_mitigations[mitigation_type] = not self.selected_mitigations[mitigation_type]

    def handle_events(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.quit(False)

            if event.type == pygame.MOUSEBUTTONDOWN:
                for view in self.views:
                    if isinstance(view, Button):
                        view.check_click(event)
                    elif isinstance(view, MitigationView):
                        view.check_click(event)

            if event.type == pygame.MOUSEBUTTONUP:
                pass

    def display_update(self):
        self.screen.fill(self.bg_c)

        for view in self.views:
            if isinstance(view, Drawable):
                view.draw(self.screen)

        pygame.display.update()
        self.clock.tick(self.fps)

    def loop(self):
        while self.running:
            try:
                self.handle_events()
                self.display_update()
            except Exception as e:
                logging.error("Setup Panel main loop error")
                break


def main():
    setup_panel = SetupPanel()
    setup_panel.loop()


if __name__ == '__main__':
    main()
