# Makefile — Ashlands engine build system
# Targets: ashlands (native), web (WASM via Emscripten), clean

CC      := gcc
EMCC    ?= emcc
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
         src/dialog_ui.c \
         src/faction.c \
         src/item.c \
         src/lua_api.c \
         src/npc.c \
         src/quest.c \
         src/save.c \
         src/texgen/noise.c \
         src/texgen/texcache.c \
         src/texgen/texgen.c \
         src/render/camera.c \
         src/render/mesh.c \
         src/render/render_3d_scene.c \
         src/render/shader.c \
         src/render/render_ascii.c \
         src/render/render.c \
         src/render/render_3d.c \
         src/procgen/dungeon.c

OBJ := $(SRC_C:.c=.o)
WEB_DIR := build/web
WEB_SHELL := web/shell/index.html
WEB_FONT := assets/fonts/DejaVuSansMono.ttf
SYSTEM_FONT := /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf

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
	@mkdir -p $(WEB_DIR) assets/fonts assets/tilesets assets/sounds mods/core
	@if [ ! -f "$(WEB_FONT)" ] && [ -f "$(SYSTEM_FONT)" ]; then \
		cp "$(SYSTEM_FONT)" "$(WEB_FONT)"; \
	fi
	@if [ ! -f "$(WEB_FONT)" ]; then \
		echo "[web] warning: $(WEB_FONT) is missing; ASCII text may not render in browser" >&2; \
	fi
	@touch assets/.keep mods/.keep
	$(EMCC) $(SRC_C) -o $(WEB_DIR)/index.html \
		-Iinclude \
		--use-port=sdl2 \
		--use-port=sdl2_ttf \
		--use-port=sdl2_image:formats=png \
		--use-port=sdl2_mixer \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s MAX_WEBGL_VERSION=2 \
		-s MIN_WEBGL_VERSION=1 \
		-s FULL_ES2=1 \
		-s ASYNCIFY \
		-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
		--preload-file assets/ \
		--preload-file mods/ \
		--shell-file $(WEB_SHELL) \
		-DPLATFORM_WEB \
		-O2
	@echo "Web build OK: $(WEB_DIR)/index.html"

# ---- Clean ---------------------------------------------------------
clean:
	rm -f $(OBJ) ashlands
	rm -rf $(WEB_DIR)

# ---- Dependency hints (optional) -----------------------------------
# Ubuntu/Debian: sudo apt install libsdl2-dev libsdl2-ttf-dev
#                libsdl2-image-dev libsdl2-mixer-dev libluajit-5.1-dev
# Arch:          sudo pacman -S sdl2 sdl2_ttf sdl2_image sdl2_mixer luajit
