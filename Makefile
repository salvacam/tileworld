CHAINPREFIX := /opt/mipsel-linux-uclibc
CROSS_COMPILE := $(CHAINPREFIX)/usr/bin/mipsel-linux-
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
STRIP = $(CROSS_COMPILE)strip

DEBUG	= 0

SYSROOT     := $(shell $(CC) --print-sysroot)
SDL_CFLAGS  := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS    := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

#CFLAGS  = -c -g -flto -Wall -D_REENTRANT -DPLATFORM_GCW $(SDL_CFLAGS)
CFLAGS  = -c -g -flto -Wall -D_REENTRANT $(SDL_CFLAGS)
# LDFLAGS = -flto -lSDL_image -lSDL_gfx -ljpeg -lpng -lz -lSDL_mixer -lvorbisidec -lvorbisfile -lvorbis -lpthread -lgcc -lm -lc $(SDL_LIBS) $(MYFLAGS)
LDFLAGS = -flto -lSDL_image -lSDL_gfx -ljpeg -lpng -lz -lSDL_mixer -lpthread -lgcc -lm -lc $(SDL_LIBS) $(MYFLAGS)
SOURCES = src/cmdline.c src/encoding.c src/err.c src/fileio.c src/help.c src/lxlogic.c src/mslogic.c src/play.c src/random.c src/res.c \
		src/score.c src/series.c src/solution.c src/tworld.c src/unslist.c \
		src/oshw-sdl/SFont.c src/oshw-sdl/ccicon.c src/oshw-sdl/sdlerr.c src/oshw-sdl/sdlin.c \
		src/oshw-sdl/sdloshw.c src/oshw-sdl/sdlout.c src/oshw-sdl/sdlsfx.c src/oshw-sdl/sdltext.c \
		src/oshw-sdl/sdltile.c src/oshw-sdl/sdltimer.c src/oshw-sdl/port_cfg.c

OBJECTS=$(SOURCES:.c=.o)
TARGET=tileworld/tileworld.dge
# EXECUTABLEGP=tworld

all: $(SOURCES) $(TARGET)

# $(TARGET): $(OBJECTS) 
# 	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(TARGET): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

ipk: $(TARGET)
	@rm -rf /tmp/.tileworld-ipk/ && mkdir -p /tmp/.tileworld-ipk/root/home/retrofw/games/tileworld /tmp/.tileworld-ipk/root/home/retrofw/apps/gmenu2x/sections/games
	@cp -r tileworld/tileworld.dge tileworld/tileworld.png tileworld/data tileworld/music tileworld/res tileworld/sets /tmp/.tileworld-ipk/root/home/retrofw/games/tileworld
	@cp tileworld/tileworld.lnk /tmp/.tileworld-ipk/root/home/retrofw/apps/gmenu2x/sections/games
	@sed "s/^Version:.*/Version: $$(date +%Y%m%d)/" tileworld/control > /tmp/.tileworld-ipk/control
	@cp tileworld/conffiles /tmp/.tileworld-ipk/
	@tar --owner=0 --group=0 -czvf /tmp/.tileworld-ipk/control.tar.gz -C /tmp/.tileworld-ipk/ control conffiles
	@tar --owner=0 --group=0 -czvf /tmp/.tileworld-ipk/data.tar.gz -C /tmp/.tileworld-ipk/root/ .
	@echo 2.0 > /tmp/.tileworld-ipk/debian-binary
	@ar r tileworld/tileworld.ipk /tmp/.tileworld-ipk/control.tar.gz /tmp/.tileworld-ipk/data.tar.gz /tmp/.tileworld-ipk/debian-binary

cleanobjs:
	rm -f $(OBJECTS)

clean:
	-rm -f $(TARGET) src/*~ src/*.o src/*.bak src/oshw-sdl/*.o src/oshw-sdl/*.bak src/oshw-sdl/*~

