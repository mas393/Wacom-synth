gui: gui.c make_sound.c
	gcc gui.c make_sound.c -o gui -lasound -lm `pkg-config --cflags --libs gtk4`
