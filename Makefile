.POSIX:
.PHONY: clean
.SUFFIXES:
.SUFFIXES: .c .o
 
rgbgfx_obj := \
	src/gfx/main.o \
	src/gfx/png.o \
	src/gfx/gb.o \
	src/extern/err.o

rgbgfx: ${rgbgfx_obj}
	$Q${CC} -o $@ ${rgbgfx_obj} `pkg-config --libs libpng`

.c.o:
	$Q${CC} -Wall -Iinclude -g `pkg-config --cflags libpng` -std=c99 -D_POSIX_C_SOURCE=200809L -c -o $@ $<

clean:
	$Qrm -rf rgbgfx ${rgbgfx_obj}

