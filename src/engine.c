/*
 * engine.c — Engine init, game loop, and subsystem glue
 * C11, MIT License
 */
#include "engine.h"
#include "faction.h"
#include "item.h"
#include "npc.h"
#include "procgen/procgen.h"
#include "quest.h"
#include "texgen/texgen.h"

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

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

static void engine_set_default_save_path(EngineState *eng) {
    snprintf(eng->save_path, sizeof(eng->save_path), "%s/ashlands.save",
             eng->cfg.data_dir[0] ? eng->cfg.data_dir : ".");
}

static void engine_sync_world_after_load(EngineState *eng) {
    PositionComponent *player;

    if (!eng || !eng->world || !eng->world->ecs) {
        return;
    }

    player = entity_pos(eng->world->ecs, eng->world->ecs->player_id);
    if (player) {
        camera_center_on(&eng->camera, (float)player->x, (float)player->y);
        map_compute_fov(&eng->world->map, player->x, player->y, 10);
    }

    eng->ui.player_id = eng->world->ecs->player_id;

    dialog_ui_render(&eng->ui, eng->world->ecs->player_id);
}

static bool engine_try_pickup(WorldState *ws) {
    World *w = ws->ecs;
    PositionComponent *player;
    InventoryComponent *inv;

    if (!w || w->player_id < 0) {
        return false;
    }

    player = entity_pos(w, w->player_id);
    inv = entity_inventory(w, w->player_id);
    if (!player || !inv) {
        return false;
    }

    for (int id = 0; id < MAX_ENTITIES; id++) {
        PositionComponent *pos;

        if (!entity_is_alive(w, id) || !entity_has(w, id, COMP_ITEM | COMP_POSITION)) {
            continue;
        }

        pos = entity_pos(w, id);
        if (!pos || pos->x != player->x || pos->y != player->y) {
            continue;
        }

        for (int slot = 0; slot < MAX_INVENTORY_SLOTS; slot++) {
            if (inv->slots[slot] < 0) {
                inv->slots[slot] = id;
                inv->count = MIN(inv->count + 1, MAX_INVENTORY_SLOTS);
                entity_rem_comp(w, id, COMP_POSITION);
                return true;
            }
        }
        return false;
    }

    return false;
}

static const char *engine_nearby_npc(WorldState *ws) {
    World *w = ws->ecs;
    PositionComponent *player;

    if (!w || w->player_id < 0) {
        return NULL;
    }

    player = entity_pos(w, w->player_id);
    if (!player) {
        return NULL;
    }

    for (int id = 0; id < MAX_ENTITIES; id++) {
        PositionComponent *pos;
        int dist;

        if (!entity_is_alive(w, id) || !entity_has_tag(w, id, "npc")) {
            continue;
        }

        pos = entity_pos(w, id);
        if (!pos) {
            continue;
        }

        dist = abs(pos->x - player->x) + abs(pos->y - player->y);
        if (dist <= 1) {
            return npc_id_for_entity(id);
        }
    }

    return NULL;
}

static bool engine_try_drop(WorldState *ws) {
    World *w = ws->ecs;
    PositionComponent *player;
    InventoryComponent *inv;

    if (!w || w->player_id < 0) {
        return false;
    }

    player = entity_pos(w, w->player_id);
    inv = entity_inventory(w, w->player_id);
    if (!player || !inv) {
        return false;
    }

    for (int slot = inv->count - 1; slot >= 0; slot--) {
        int item_id = inv->slots[slot];
        if (item_id < 0 || !entity_is_alive(w, item_id)) {
            continue;
        }

        entity_add_comp(w, item_id, COMP_POSITION);
        w->positions[item_id].x = player->x;
        w->positions[item_id].y = player->y;
        w->positions[item_id].z = 0;
        inv->slots[slot] = -1;
        inv->count = MAX(inv->count - 1, 0);
        return true;
    }

    return false;
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
    rc->texture_id[0] = '\0';
    rc->mesh_id[0] = '\0';

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

static void seed_player_inventory(WorldState *ws) {
    World *w = ws->ecs;
    InventoryComponent *inv = entity_inventory(w, w->player_id);

    if (!inv || inv->count > 0) {
        return;
    }

    item_spawn(w, "bandage", 0, 0, true, 0, 1);
    item_spawn(w, "raw_meat", 0, 0, true, 1, 1);
    inv->count = 2;
}

static void seed_world_items(WorldState *ws) {
    World *w = ws->ecs;
    int px = ws->map.spawn_x;
    int py = ws->map.spawn_y;

    item_spawn(w, "wolf_pelt", px + 1, py + 1, false, -1, 1);
    item_spawn(w, "ash_fang", px + 2, py + 1, false, -1, 1);
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

    npc_spawn(eng->world->ecs, "grom_hunter", px + 2, py);
    seed_player_inventory(eng->world);
    seed_world_items(eng->world);

    ui_log(&eng->ui, "В пепельных пустошах нет героев.", COL_GRAY);
    ui_log(&eng->ui, "Есть только те, кто ещё не сдался.", COL_WHITE);
    ui_log(&eng->ui, "Используй стрелки или WASD для движения.", COL_GRAY);
}

static bool engine_uses_opengl(RenderMode mode) {
    return mode == RENDER_LOWPOLY_3D;
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
    engine_set_default_save_path(eng);

    Uint32 win_flags = SDL_WINDOW_RESIZABLE;
    if (cfg->fullscreen) win_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (engine_uses_opengl(cfg->render_mode)) {
        win_flags |= SDL_WINDOW_OPENGL;
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#ifdef PLATFORM_WEB
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
    }

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
    eng->renderer = renderer_create(cfg->render_mode);
    renderer_set(eng->renderer, eng->window, cfg->screen_w, cfg->screen_h);
    if (engine_uses_opengl(cfg->render_mode)) {
        SDL_GL_SetSwapInterval(cfg->vsync ? 1 : 0);
    }

    /* Lua */
    eng->lua = lua_ctx_create();
    if (eng->lua) {
        faction_init();
        faction_set_world(eng->world);
        item_registry_init();
        npc_init();
        quest_init();
        quest_set_world(eng->world);
        texgen_init();
        lua_api_register(eng->lua);
        lua_ctx_set_world(eng->lua, eng->world);
        lua_api_set_ui_log(engine_ui_log_cb);
        /* Load core mods */
        lua_ctx_load_file(eng->lua, "mods/core/init.lua");
    }

    input_init(&eng->input);

    init_world(eng);
    load_game(eng->save_path, eng);
    engine_sync_world_after_load(eng);

    eng->running   = true;
    eng->last_tick = SDL_GetTicks();
    return eng;
}

void engine_destroy(EngineState *eng) {
    if (!eng) return;
    if (eng->renderer && eng->renderer->shutdown) eng->renderer->shutdown();
    quest_shutdown();
    npc_shutdown();
    item_registry_shutdown();
    faction_shutdown();
    texgen_shutdown();
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

    if (action == ACTION_SAVE_GAME) {
        if (save_game(eng->save_path, eng)) {
            ui_log(&eng->ui, "Игра сохранена.", COL_GREEN);
        } else {
            ui_log(&eng->ui, "Не удалось сохранить игру.", COL_RED);
        }
    }

    if (action == ACTION_LOAD_GAME) {
        if (load_game(eng->save_path, eng)) {
            engine_sync_world_after_load(eng);
            ui_log(&eng->ui, "Игра загружена.", COL_GREEN);
        } else {
            ui_log(&eng->ui, "Не удалось загрузить игру.", COL_RED);
        }
    }

    if (action == ACTION_INTERACT) {
        const char *npc_id = engine_nearby_npc(eng->world);

        if (npc_dialog_open()) {
            npc_select_option(eng->world->ecs->player_id, 0);
            dialog_ui_render(&eng->ui, eng->world->ecs->player_id);
        } else if (npc_id && npc_start_dialog(eng->world->ecs->player_id, npc_id)) {
            quest_on_talk_to(npc_id);
            dialog_ui_render(&eng->ui, eng->world->ecs->player_id);
        } else {
            ui_log(&eng->ui, "Рядом никого нет.", COL_GRAY);
        }
    }

    if (npc_dialog_open() &&
        action >= ACTION_DIALOG_OPTION_1 &&
        action <= ACTION_DIALOG_OPTION_8) {
        int option_index = (int)action - (int)ACTION_DIALOG_OPTION_1;
        npc_select_option(eng->world->ecs->player_id, option_index);
        dialog_ui_render(&eng->ui, eng->world->ecs->player_id);
    }

    if (npc_dialog_open() && action == ACTION_CANCEL) {
        npc_close_dialog();
        dialog_ui_render(&eng->ui, eng->world->ecs->player_id);
    }

    if (action == ACTION_PICKUP) {
        const char *picked_id = NULL;
        World *w = eng->world->ecs;
        InventoryComponent *inv = entity_inventory(w, w->player_id);

        if (engine_try_pickup(eng->world)) {
            if (inv && inv->count > 0) {
                picked_id = NULL;
                for (int slot = 0; slot < MAX_INVENTORY_SLOTS; slot++) {
                    if (inv->slots[slot] >= 0) {
                        ItemComponent *item = entity_item(w, inv->slots[slot]);
                        if (item && item->stack_count > 0) {
                            picked_id = item->id;
                        }
                    }
                }
            }
            ui_log(&eng->ui, "Предмет поднят.", COL_YELLOW);
            if (picked_id) {
                quest_on_collect(picked_id, 1);
            }
        } else {
            ui_log(&eng->ui, "Здесь нечего поднимать.", COL_GRAY);
        }
    }

    if (action == ACTION_DROP) {
        if (engine_try_drop(eng->world)) {
            ui_log(&eng->ui, "Предмет брошен.", COL_YELLOW);
        } else {
            ui_log(&eng->ui, "Инвентарь пуст.", COL_GRAY);
        }
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

    eng->ui.fps = eng->fps;
    r->render_ui(&eng->ui);

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
