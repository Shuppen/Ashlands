# Makefile — Ashlands engine build system
# Targets: ashlands (native), web (WASM via Emscripten), clean

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -O2 \
           -Iinclude \
           $(shell sdl2-config --cflags 2>/dev/null || echo "-I/usr/include/SDL2") \
           -I/usr/include/luajit-2.1 \
           -I/usr/local/include/luajit-2.1

LDFLAGS := $(shell sdl2-config --libs 2>/dev/null || echo "-lSDL2") \
           -lSDL2_image -lSDL2_ttf -lSDL2_mixer \
           -lluajit-5.1 \
           -lGL -lm -ldl

# ---- Source files --------------------------------------------------
SRC_C := src/main.c \
         src/engine.c \
         src/ecs.c \
         src/world.c \
         src/input.c \
         src/ui.c \
         src/lua_api.c \
         src/render/camera.c \
         src/render/render_ascii.c \
         src/procgen/dungeon.c

OBJ := $(SRC_C:.c=.o)

# ---- Default target ------------------------------------------------
.PHONY: all clean web run

all: ashlands

ashlands: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)
	@echo "Build OK: ashlands"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Run -----------------------------------------------------------
run: ashlands
	./ashlands

# ---- Web build (Emscripten) ----------------------------------------
web:
	@mkdir -p web
	emcc $(SRC_C) -o web/index.html \
		-Iinclude \
		-s USE_SDL=2 \
		-s USE_SDL_TTF=2 \
		-s USE_SDL_IMAGE=2 \
		-s USE_SDL_MIXER=2 \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s MAX_WEBGL_VERSION=2 \
		-s FULL_ES2=1 \
		--preload-file assets/ \
		--preload-file mods/ \
		-O2
	@echo "Web build OK: web/index.html"

# ---- Clean ---------------------------------------------------------
clean:
	rm -f $(OBJ) ashlands
	rm -rf web/

# ---- Dependency hints (optional) -----------------------------------
# Ubuntu/Debian: sudo apt install libsdl2-dev libsdl2-ttf-dev
#                libsdl2-image-dev libsdl2-mixer-dev libluajit-5.1-dev
# Arch:          sudo pacman -S sdl2 sdl2_ttf sdl2_image sdl2_mixer luajit
