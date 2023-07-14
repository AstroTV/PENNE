#!/bin/bash
gcc -Wall -Wextra src/main.c src/helper.c src/bluetooth.c $(pkg-config --libs --cflags glib-2.0 gio-2.0) -lncurses -o app
./app "$1"l