all: main.c
	gcc main.c `pkg-config --cflags --libs gtk+-3.0 gmodule-2.0`
debug: main.c
	gcc main.c -DDEBUG -g `pkg-config --cflags --libs gtk+-3.0 gmodule-2.0`
