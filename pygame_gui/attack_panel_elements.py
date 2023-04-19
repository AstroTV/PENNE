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

    def on_click(self, event):
        if event.button == 1:
            if self.rect.collidepoint(event.pos):
                self.callback()
                self.image.set_alpha(50)

    def on_release(self, event):
        if event.button == 1:
            self.image.set_alpha(255)


class ToggleButton(Button):
    def __init__(self, path_a, path_b, pos_x, pos_y, callback):
        super().__init__(path_a, pos_x, pos_y, callback)
        self.image_a = pygame.image.load(path_a)
        self.image_b = pygame.image.load(path_b)
        self.state = 0

    def toggle(self):
        if self.state == 0:
            self.state = 1
            self.image = self.image_b
        elif self.state == 1:
            self.state = 0
            self.image = self.image_a
