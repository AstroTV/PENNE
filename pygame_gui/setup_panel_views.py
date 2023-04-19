from constants import *
from base_views import Drawable, TextField, Button


class MitigationView(Drawable):

    def __init__(self, pos_x, pos_y, image_path, title_text, description_text, screen, select_callback,
                 mitigation_type):
        super().__init__(None, pos_x, pos_y)
        self.__screen = screen
        self.selected = False
        self.__select_callback = select_callback
        self.__image = Button(image_path, pos_x, pos_y, self.on_toggle_selection)
        self.__selected_indicator = Drawable(get_asset_path("setup_select_mitigation_type.svg"), pos_x, pos_y)
        title = TextField(self.pos_x + 40, self.pos_y, title_text,
                          SETUP_MITIGATION_TITLE_COLOR, SETUP_MITIGATION_TITLE_FONT,
                          SETUP_MITIGATION_TITLE_FONT_SIZE)

        description = TextField(self.pos_x + 40, self.pos_y + title.font_size, description_text,
                                SETUP_MITIGATION_DESCRIPTION_COLOR, SETUP_MITIGATION_DESCRIPTION_FONT,
                                SETUP_MITIGATION_DESCRIPTION_FONT_SIZE)

        self.mitigation_type = mitigation_type

        self.elements = {
            "image": self.__image,
            "title": title,
            "description": description,
        }

    def draw(self, screen):
        if self.selected:
            self.__selected_indicator.draw(self.__screen)

        for key, value in self.elements.items():
            value.draw(self.__screen)

    def check_click(self, event):
        self.__image.check_click(event)

    def on_toggle_selection(self):
        self.selected = not self.selected
        self.__select_callback(self.mitigation_type)
