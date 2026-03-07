/*
 * engine.h — Engine init, config, and main-loop interface
 * C11, MIT License
 */
#ifndef ASHLANDS_ENGINE_H
#define ASHLANDS_ENGINE_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#include "../include/ashlands.h"
#include "world.h"
#include "input.h"
#include "lua_api.h"
#include "ui.h"
#include "render/render.h"
#include "render/camera.h"

/* =========================================================
 * Engine configuration (loaded from config.lua or defaults)
 * ========================================================= */
typedef struct {
    int         screen_w;
    int         screen_h;
    bool        fullscreen;
    bool        vsync;
    RenderMode  render_mode;
    int         target_fps;
    uint32_t    world_seed;
    char        data_dir[256];
} EngineConfig;

/* =========================================================
 * EngineState — owns all subsystems
 * ========================================================= */
typedef struct {
    SDL_Window   *window;
    bool          running;

    EngineConfig  cfg;
    WorldState   *world;
    InputState    input;
    UIState       ui;
    LuaContext   *lua;
    Camera        camera;
    Renderer     *renderer;

    /* FPS tracking */
    uint32_t      last_tick;
    int           frame_count;
    int           fps;
} EngineState;

/* =========================================================
 * Lifecycle
 * ========================================================= */
EngineState *engine_create(const EngineConfig *cfg);
void         engine_destroy(EngineState *eng);

/* Default config (can be overridden before engine_create) */
EngineConfig engine_default_config(void);

/* Run the game loop until the engine stops */
void         engine_run(EngineState *eng);

/* Stop the engine (can be called from input handler or Lua) */
void         engine_stop(EngineState *eng);

#endif /* ASHLANDS_ENGINE_H */
