import os
from pygame import color, font

#
COLOR_MAIN_BACKGROUND = (100, 100, 100)

BASE_PROJECT_PATH = os.path.dirname(os.path.dirname(__file__))
BASE_GUI_PATH = os.path.dirname(__file__)
ASSETS_PATH = BASE_GUI_PATH + "/assets/"

# strings to display
SETUP_PANEL_TITLE = "Setup Panel"
SETUP_NONE_TITLE = "None"
SETUP_NONE_DESCRIPTION_SHORT = ""
SETUP_NONE_DESCRIPTION = ""
SETUP_ENCRYPTION_TITLE = "Encryption"
SETUP_ENCRYPTION_DESCRIPTION_SHORT = ""
SETUP_ENCRYPTION_DESCRIPTION = ""
SETUP_GATEWAY_TITLE = "Gateway"
SETUP_GATEWAY_DESCRIPTION_SHORT = ""
SETUP_GATEWAY_DESCRIPTION = ""
SETUP_MONITORING_TITLE = "Monitoring"
SETUP_MONITORING_DESCRIPTION_SHORT = ""
SETUP_MONITORING_DESCRIPTION = ""

# Colors
SETUP_TITLE_COLOR = color.Color('black')
SETUP_MITIGATION_TITLE_COLOR = color.Color('black')
SETUP_MITIGATION_DESCRIPTION_COLOR = color.Color('black')

# Fonts
SETUP_TITLE_FONT = font.get_default_font()
SETUP_MITIGATION_TITLE_FONT = font.get_default_font()
SETUP_MITIGATION_DESCRIPTION_FONT = font.get_default_font()

# Font sizes
SETUP_TITLE_FONT_SIZE = 40
SETUP_MITIGATION_TITLE_FONT_SIZE = 30
SETUP_MITIGATION_DESCRIPTION_FONT_SIZE = 20

# helper fucntions


def get_asset_path(name: str) -> str:
    return ASSETS_PATH + name

