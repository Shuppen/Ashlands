# Contributing to Ashlands

Welcome! Ashlands is built around the idea that **anyone** can contribute —
whether you're a seasoned C developer or a vibe-coder using AI assistants.

---

## Types of contributions

| What | How | Where |
|------|-----|-------|
| New creature | Lua mod | `mods/community/` |
| New item | Lua mod | `mods/community/` |
| New biome | Lua mod | `mods/community/` |
| New quest | Lua mod | `mods/community/` |
| New NPC dialog | Lua mod | `mods/community/` |
| New procedural texture | Lua mod | `mods/community/` |
| Bug fix in engine | C pull request | `src/` |
| New engine feature | C pull request | `src/` |
| Tileset / art | PNG assets | `assets/tilesets/` |
| Sound effects | WAV files | `assets/sounds/` |
| Music | OGG files | `assets/music/` |
| Documentation | Markdown | `templates/`, `README.md` |

---

## For vibe-coders (Lua content)

Ты не должен уметь программировать, чтобы сделать полезный вклад. Если можешь описать идею словами, AI уже может превратить её в мод на Lua.

1. Read `templates/README.md` — it explains everything
2. Copy the relevant template from `templates/`
3. Fill it in (ask your AI assistant if needed)
4. Place it in `mods/community/<your_mod>/`
5. Test it: run `./ashlands` and check the log for Lua errors
6. Submit a pull request

**You don't need to understand C to contribute Lua content.**

Для Phase 3 особенно полезны квесты, фракции, NPC-диалоги и процедурные текстуры.

### Уровни сложности

- ⭐ Предмет или простая текстура
- ⭐⭐ Существо или короткий диалог
- ⭐⭐⭐ Биом или многошаговый квест
- ⭐⭐⭐⭐ Цепочка квестов / фракционный контент
- ⭐⭐⭐⭐⭐ Система движка, Android, renderer, tooling

### Как работать с AI

1. Скопируй нужный шаблон из `templates/`
2. Опиши желаемый контент обычным языком
3. Попроси AI заполнить шаблон, не меняя формат API
4. Проверь Lua-ошибки в игре
5. Открой PR или issue

---

## For engine developers (C code)

### Setup

```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev \
                 libsdl2-mixer-dev libluajit-5.1-dev libgl1-mesa-dev

# Arch
sudo pacman -S sdl2 sdl2_ttf sdl2_image sdl2_mixer luajit mesa

# Build
make
./ashlands
```

### Code style

- **Language:** Pure C11 (no C++)
- **Naming:** `snake_case` for everything
- **File size:** max 500 lines per `.c` file — split if larger
- **Memory:** no leaks. Every `malloc` has a matching `free`
- **Portability:** no `system()`, `fork()`, `popen()`
- **Files:** use `SDL_RWops` or `fopen` — no platform-specific IO
- **OpenGL:** max 2.0 / GLSL 1.20
- **Warnings:** code must compile with `-Wall -Wextra` without warnings

### Pull request checklist

- [ ] Code compiles with `make` (gcc) and `cmake` (clang)
- [ ] No new warnings
- [ ] No memory leaks (test with `valgrind ./ashlands` if possible)
- [ ] Each `.c` file is ≤ 500 lines
- [ ] Lua API additions are documented in `templates/README.md`

### Architecture overview

```
src/
├── main.c           — entry point, arg parsing
├── engine.c/h       — game loop, subsystem glue
├── ecs.c/h          — entity-component-system
├── world.c/h        — map, chunks, time, weather
├── input.c/h        — keyboard / mouse / gamepad
├── ui.c/h           — HUD, log, menus
├── lua_api.c/h      — Lua bindings (C→Lua interface)
├── render/
│   ├── render.h     — abstract renderer interface
│   ├── camera.c/h   — camera (all modes)
│   ├── render_ascii.c  — ASCII backend (Phase 1 ✓)
│   ├── render_tiles.c  — 2D tiles (Phase 2)
│   ├── render_iso.c    — isometric (Phase 2)
│   └── render_3d.c     — low-poly 3D (Phase 3)
├── faction.c/h      — фракции и репутация
├── npc.c/h          — NPC и диалоги
├── quest.c/h        — квесты и стадии
├── texgen/
│   ├── texgen.c/h   — procedural texture generator
│   └── noise.c/h    — Perlin, Voronoi, Cellular noise
└── procgen/
    ├── dungeon.c    — BSP dungeon generator
    └── overworld.c  — overworld generator (Phase 2)
```

---

## Community

- Issues & discussions: GitHub Issues
- Devlog: see `README.md` for links

## Code of Conduct

- Будь уважителен к новичкам и вайб-кодерам
- Критикуй код и идею, а не человека
- Не публикуй чужие приватные ассеты, ключи и секреты
- Если не уверен — открой issue, а не спор

*«В пепельных пустошах нет героев. Есть только те, кто ещё не сдался.»*
