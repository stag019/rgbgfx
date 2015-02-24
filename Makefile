.POSIX:
.PHONY: clean
.SUFFIXES:
.SUFFIXES: .c .o

PREFIX =	/usr/local
BINPREFIX =	${PREFIX}/bin
 
rgbgfx_obj := \
	src/gfx/main.o \
	src/gfx/png.o \
	src/gfx/gb.o \
	src/extern/err.o

rgbgfx: ${rgbgfx_obj}
	$Q${CC} -o $@ ${rgbgfx_obj} `pkg-config --libs libpng`

clean:
	$Qrm -rf rgbgfx ${rgbgfx_obj}

install: rgbgfx
	$Qmkdir -p ${BINPREFIX}
	$Qinstall -s -m 555 rgbgfx ${BINPREFIX}/rgbgfx

.c.o:
	$Q${CC} -Wall -Iinclude -g `pkg-config --cflags libpng` -std=c99 -D_POSIX_C_SOURCE=200809L -c -o $@ $<

