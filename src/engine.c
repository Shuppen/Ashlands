/*
 * engine.c — Engine init, game loop, and subsystem glue
 * C11, MIT License
 */
#include "engine.h"
#include "procgen/procgen.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

/* Forward declaration for ASCII renderer helpers */
SDL_Renderer *render_ascii_get_sdl_renderer(void);
TTF_Font     *render_ascii_get_font(void);

/* =========================================================
 * UI log callback wired from Lua
 * ========================================================= */
static UIState *g_ui_ptr = NULL;

#ifdef PLATFORM_WEB
static EngineState *g_web_engine = NULL;
#endif

static void engine_ui_log_cb(const char *text, uint32_t color) {
    if (g_ui_ptr) ui_log(g_ui_ptr, text, color);
}

/* =========================================================
 * Config defaults
 * ========================================================= */
EngineConfig engine_default_config(void) {
    EngineConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.screen_w    = 1024;
    cfg.screen_h    = 768;
    cfg.fullscreen  = false;
    cfg.vsync       = true;
    cfg.render_mode = RENDER_ASCII;
    cfg.target_fps  = 60;
    cfg.world_seed  = (uint32_t)time(NULL);
    strncpy(cfg.data_dir, ".", sizeof(cfg.data_dir) - 1);
    return cfg;
}

/* =========================================================
 * Player spawn
 * ========================================================= */
static int spawn_player(WorldState *ws, int x, int y) {
    World *w  = ws->ecs;
    int player = entity_create(w);
    if (player < 0) return -1;

    entity_add_comp(w, player,
        COMP_POSITION | COMP_HEALTH | COMP_RENDER |
        COMP_STATS    | COMP_INVENTORY | COMP_PLAYER);

    PositionComponent *pos = entity_pos(w, player);
    pos->x = x; pos->y = y; pos->z = 0;

    HealthComponent *hp = entity_health(w, player);
    hp->current = hp->max = 100;

    RenderComponent *rc = entity_render(w, player);
    rc->glyph    = '@';
    rc->fg_color = COL_WHITE;
    rc->bg_color = COL_BLACK;
    rc->sprite_id = -1;

    StatsComponent *st = entity_stats(w, player);
    st->attack  = 5;
    st->defense = 3;
    st->speed   = 5;
    st->level   = 1;
    st->xp      = 0;

    InventoryComponent *inv = entity_inventory(w, player);
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) inv->slots[i] = -1;
    inv->count = 0;

    w->player_id = player;
    return player;
}

/* =========================================================
 * World initialisation
 * ========================================================= */
static void init_world(EngineState *eng) {
    DungeonParams dp = {
        .map_w      = 80,
        .map_h      = 50,
        .min_room_w = 5,
        .min_room_h = 4,
        .max_room_w = 15,
        .max_room_h = 12,
        .max_depth  = 5,
        .seed       = eng->cfg.world_seed
    };
    dungeon_generate(&eng->world->map, eng->world, &dp);

    int px = eng->world->map.spawn_x;
    int py = eng->world->map.spawn_y;
    int player_id = spawn_player(eng->world, px, py);

    ui_state_init(&eng->ui, player_id);
    g_ui_ptr = &eng->ui;

    camera_init(&eng->camera,
                eng->cfg.screen_w, eng->cfg.screen_h,
                16, 16);
    camera_center_on(&eng->camera, (float)px, (float)py);

    /* Initial FOV */
    map_compute_fov(&eng->world->map, px, py, 10);

    ui_log(&eng->ui, "В пепельных пустошах нет героев.", COL_GRAY);
    ui_log(&eng->ui, "Есть только те, кто ещё не сдался.", COL_WHITE);
    ui_log(&eng->ui, "Используй стрелки или WASD для движения.", COL_GRAY);
}

/* =========================================================
 * Engine lifecycle
 * ========================================================= */
EngineState *engine_create(const EngineConfig *cfg) {
#ifdef PLATFORM_WEB
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "[engine] SDL_Init: %s\n", SDL_GetError());
        return NULL;
    }

    EngineState *eng = calloc(1, sizeof(EngineState));
    if (!eng) return NULL;

    eng->cfg = *cfg;

    Uint32 win_flags = SDL_WINDOW_RESIZABLE;
    if (cfg->fullscreen) win_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    eng->window = SDL_CreateWindow(
        "Ashlands " ASHLANDS_VERSION_STR,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        cfg->screen_w, cfg->screen_h,
        win_flags);

    if (!eng->window) {
        fprintf(stderr, "[engine] SDL_CreateWindow: %s\n", SDL_GetError());
        free(eng);
        SDL_Quit();
        return NULL;
    }

    /* World */
    eng->world = world_state_create(cfg->world_seed);
    if (!eng->world) {
        fprintf(stderr, "[engine] world_state_create failed\n");
        SDL_DestroyWindow(eng->window);
        free(eng);
        SDL_Quit();
        return NULL;
    }

    /* Renderer */
    eng->renderer = render_ascii_create();
    renderer_set(eng->renderer, eng->window, cfg->screen_w, cfg->screen_h);

    /* Lua */
    eng->lua = lua_ctx_create();
    if (eng->lua) {
        lua_api_register(eng->lua);
        lua_ctx_set_world(eng->lua, eng->world);
        /* Wire UI log callback */
        extern void lua_api_set_ui_log(void (*fn)(const char*, uint32_t));
        lua_api_set_ui_log(engine_ui_log_cb);
        /* Load core mods */
        lua_ctx_load_file(eng->lua, "mods/core/init.lua");
    }

    input_init(&eng->input);

    init_world(eng);

    eng->running   = true;
    eng->last_tick = SDL_GetTicks();
    return eng;
}

void engine_destroy(EngineState *eng) {
    if (!eng) return;
    if (eng->renderer && eng->renderer->shutdown) eng->renderer->shutdown();
    lua_ctx_destroy(eng->lua);
    world_state_destroy(eng->world);
    SDL_DestroyWindow(eng->window);
    SDL_Quit();
    free(eng);
}

void engine_stop(EngineState *eng) {
    if (eng) eng->running = false;
}

/* =========================================================
 * Per-frame update logic
 * ========================================================= */
static void engine_update(EngineState *eng) {
    GameAction action = input_get_action(&eng->input);

    if (eng->input.quit_requested || action == ACTION_QUIT) {
        eng->running = false;
        return;
    }

    if (eng->input.window_resized) {
        eng->cfg.screen_w = eng->input.new_width;
        eng->cfg.screen_h = eng->input.new_height;
        eng->camera.screen_w = eng->input.new_width;
        eng->camera.screen_h = eng->input.new_height;
        if (eng->renderer->on_resize)
            eng->renderer->on_resize(eng->input.new_width, eng->input.new_height);
    }

    if (action == ACTION_TOGGLE_FULLSCREEN) {
        Uint32 flags = SDL_GetWindowFlags(eng->window);
        if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
            SDL_SetWindowFullscreen(eng->window, 0);
        else
            SDL_SetWindowFullscreen(eng->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }

    /* Player movement */
    int dx = 0, dy = 0;
    if (action_to_dir(action, &dx, &dy)) {
        World *w = eng->world->ecs;
        int pid  = w->player_id;
        if (pid >= 0 && entity_has(w, pid, COMP_POSITION)) {
            PositionComponent *pos = entity_pos(w, pid);
            int nx = pos->x + dx;
            int ny = pos->y + dy;

            if (map_is_walkable(&eng->world->map, nx, ny)) {
                pos->x = nx;
                pos->y = ny;
                camera_center_on(&eng->camera,
                                  (float)nx, (float)ny);
                map_compute_fov(&eng->world->map, nx, ny, 10);
                world_time_tick(eng->world);
            } else {
                /* Bump into a door — open it */
                Tile *t = map_tile(&eng->world->map, nx, ny);
                if (t && t->type == TILE_DOOR_SHUT) {
                    t->type = TILE_DOOR_OPEN;
                    ui_log(&eng->ui, "Дверь открыта.", COL_BROWN);
                }
            }
        }
    }
}

/* =========================================================
 * Render one frame
 * ========================================================= */
static void engine_render(EngineState *eng) {
    Renderer *r = eng->renderer;
    if (!r) return;

    r->begin_frame();
    r->render_map(eng->world, &eng->camera);
    r->render_entities(eng->world, &eng->camera);

    /* UI rendered directly via SDL */
    SDL_Renderer *sdl_ren = render_ascii_get_sdl_renderer();
    TTF_Font     *font    = render_ascii_get_font();
    eng->ui.fps = eng->fps;
    ui_render(&eng->ui, sdl_ren, font,
              eng->cfg.screen_w, eng->cfg.screen_h);

    r->end_frame();
}

static void engine_handle_events(EngineState *eng) {
    input_begin_frame(&eng->input);

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        input_handle_event(&eng->input, &ev);
    }
}

static void engine_update_fps(EngineState *eng) {
    eng->frame_count++;

    uint32_t now = SDL_GetTicks();
    if (now - eng->last_tick >= 1000) {
        eng->fps = eng->frame_count;
        eng->frame_count = 0;
        eng->last_tick = now;
    }
}

static void engine_run_frame(EngineState *eng, int frame_ms, bool cap_fps) {
    uint32_t frame_start = SDL_GetTicks();

    engine_handle_events(eng);
    engine_update(eng);

    if (!eng->running) {
#ifdef PLATFORM_WEB
        emscripten_cancel_main_loop();
        g_web_engine = NULL;
        engine_destroy(eng);
#endif
        return;
    }

    engine_render(eng);

    if (cap_fps) {
        uint32_t elapsed = SDL_GetTicks() - frame_start;
        if (frame_ms > 0 && (int)elapsed < frame_ms)
            SDL_Delay(frame_ms - (int)elapsed);
    }

    engine_update_fps(eng);
}

#ifdef PLATFORM_WEB
static void engine_web_frame(void) {
    if (!g_web_engine) return;
    engine_run_frame(g_web_engine, 0, false);
}
#endif

/* =========================================================
 * Main loop
 * ========================================================= */
void engine_run(EngineState *eng) {
    const int frame_ms = eng->cfg.target_fps > 0 ? 1000 / eng->cfg.target_fps : 0;

#ifdef PLATFORM_WEB
    g_web_engine = eng;
    emscripten_set_main_loop(engine_web_frame, 0, 1);
#else
    while (eng->running) {
        engine_run_frame(eng, frame_ms, true);
    }
#endif
}
