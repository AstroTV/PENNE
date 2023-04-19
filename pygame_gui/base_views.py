import pygame


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


class Button(Drawable):
    def __init__(self, path, pos_x, pos_y, callback):
        super().__init__(path, pos_x, pos_y)
        self.rect = self.image.get_rect(
            topleft=(pos_x - self.image.get_size()[0] / 2, pos_y - self.image.get_size()[1] / 2))
        self.callback = callback

    def check_click(self, event):
        if event.button == 1:
            if self.rect.collidepoint(event.pos):
                self.on_click()

    def on_click(self):
        self.callback()


class TextField(Drawable):

    def __init__(self, pos_x, pos_y, text, color, font, font_size):
        super().__init__(None, pos_x, pos_y)
        self.pos_x = pos_x
        self.pos_y = pos_y
        self.font_size = font_size
        font = pygame.font.SysFont(font, font_size)
        self.image = font.render(text, True, color)
