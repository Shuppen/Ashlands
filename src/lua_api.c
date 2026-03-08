/*
 * lua_api.c — LuaJIT bindings implementation
 * C11, MIT License
 *
 * All functions exported to Lua follow the signature:
 *   static int l_function_name(lua_State *L)
 * They push results and return the result count.
 */
#include "lua_api.h"

#if defined(PLATFORM_WEB) || defined(PLATFORM_ANDROID)

#include <stdint.h>
#include <stdlib.h>

LuaContext *lua_ctx_create(void) {
    return calloc(1, sizeof(LuaContext));
}

void lua_ctx_destroy(LuaContext *ctx) {
    free(ctx);
}

bool lua_ctx_load_file(LuaContext *ctx, const char *path) {
    (void)ctx;
    (void)path;
    return false;
}

int lua_ctx_load_dir(LuaContext *ctx, const char *dir_path) {
    (void)ctx;
    (void)dir_path;
    return 0;
}

void lua_ctx_set_world(LuaContext *ctx, void *world_state) {
    (void)ctx;
    (void)world_state;
}

void lua_api_register(LuaContext *ctx) {
    (void)ctx;
}

void lua_api_set_ui_log(void (*fn)(const char *, uint32_t)) {
    (void)fn;
}

int lua_api_ref_function(struct lua_State *L, int index) {
    (void)L;
    (void)index;
    return -2;
}

void lua_api_unref_function(int ref) {
    (void)ref;
}

bool lua_api_call_ref_bool(int ref) {
    (void)ref;
    return true;
}

bool lua_api_call_ref_void(int ref) {
    (void)ref;
    return true;
}

bool lua_api_call_ref_int(int ref, int value) {
    (void)ref;
    (void)value;
    return true;
}

bool lua_call_on_turn(LuaContext *ctx, int entity_id) {
    (void)ctx;
    (void)entity_id;
    return false;
}

bool lua_call_on_death(LuaContext *ctx, int entity_id) {
    (void)ctx;
    (void)entity_id;
    return false;
}

bool lua_call_on_see(LuaContext *ctx, int entity_id, int target_id) {
    (void)ctx;
    (void)entity_id;
    (void)target_id;
    return false;
}

bool lua_call_on_interact(LuaContext *ctx, int entity_id, int other_id) {
    (void)ctx;
    (void)entity_id;
    (void)other_id;
    return false;
}

#else

#include "world.h"
#include "ecs.h"
#include "faction.h"
#include "item.h"
#include "npc.h"
#include "quest.h"
#include "texgen/texgen.h"

#include "lua_compat.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* =========================================================
 * Global WorldState pointer (set by lua_ctx_set_world)
 * ========================================================= */
static WorldState *g_world = NULL;
static lua_State *g_api_lua = NULL;

void lua_ctx_set_world(LuaContext *ctx, void *ws) {
    (void)ctx;
    g_world = (WorldState *)ws;
}

int lua_api_ref_function(struct lua_State *L, int index) {
    if (!L || !lua_isfunction(L, index)) {
        return LUA_NOREF;
    }
    lua_pushvalue(L, index);
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

void lua_api_unref_function(int ref) {
    if (!g_api_lua || ref == LUA_NOREF || ref == LUA_REFNIL) {
        return;
    }
    luaL_unref(g_api_lua, LUA_REGISTRYINDEX, ref);
}

bool lua_api_call_ref_bool(int ref) {
    bool result = false;

    if (!g_api_lua || ref == LUA_NOREF || ref == LUA_REFNIL) {
        return true;
    }

    lua_rawgeti(g_api_lua, LUA_REGISTRYINDEX, ref);
    if (lua_pcall(g_api_lua, 0, 1, 0) != 0) {
        fprintf(stderr, "[Lua] callback: %s\n", lua_tostring(g_api_lua, -1));
        lua_pop(g_api_lua, 1);
        return false;
    }
    result = lua_toboolean(g_api_lua, -1) != 0;
    lua_pop(g_api_lua, 1);
    return result;
}

bool lua_api_call_ref_void(int ref) {
    if (!g_api_lua || ref == LUA_NOREF || ref == LUA_REFNIL) {
        return true;
    }

    lua_rawgeti(g_api_lua, LUA_REGISTRYINDEX, ref);
    if (lua_pcall(g_api_lua, 0, 0, 0) != 0) {
        fprintf(stderr, "[Lua] callback: %s\n", lua_tostring(g_api_lua, -1));
        lua_pop(g_api_lua, 1);
        return false;
    }
    return true;
}

bool lua_api_call_ref_int(int ref, int value) {
    if (!g_api_lua || ref == LUA_NOREF || ref == LUA_REFNIL) {
        return true;
    }

    lua_rawgeti(g_api_lua, LUA_REGISTRYINDEX, ref);
    lua_pushinteger(g_api_lua, value);
    if (lua_pcall(g_api_lua, 1, 0, 0) != 0) {
        fprintf(stderr, "[Lua] callback: %s\n", lua_tostring(g_api_lua, -1));
        lua_pop(g_api_lua, 1);
        return false;
    }
    return true;
}

/* =========================================================
 * Helper macros
 * ========================================================= */
#define CHECK_WORLD(L) \
    do { if (!g_world) { \
        lua_pushstring((L), "world not initialized"); \
        lua_error((L)); \
    } } while (0)

/* =========================================================
 * === ENTITIES ===
 * ========================================================= */

/* entity_spawn(type_id, x, y) → entity_id */
static int l_entity_spawn(lua_State *L) {
    CHECK_WORLD(L);
    /* type_id is a string key for creature registry (Phase 2) */
    /* const char *type_id = luaL_checkstring(L, 1); */
    int x = (int)luaL_checknumber(L, 2);
    int y = (int)luaL_checknumber(L, 3);

    World *w = g_world->ecs;
    int id = entity_create(w);
    if (id < 0) { lua_pushinteger(L, -1); return 1; }

    entity_add_comp(w, id, COMP_POSITION | COMP_RENDER | COMP_HEALTH);
    PositionComponent *pos = entity_pos(w, id);
    pos->x = x; pos->y = y; pos->z = 0;

    HealthComponent *hp = entity_health(w, id);
    hp->current = hp->max = 10;

    RenderComponent *rc = entity_render(w, id);
    rc->glyph    = 'e';
    rc->fg_color = COL_RED;
    rc->bg_color = COL_BLACK;
    rc->sprite_id = -1;

    lua_pushinteger(L, id);
    return 1;
}

/* entity_destroy(entity_id) */
static int l_entity_destroy(lua_State *L) {
    CHECK_WORLD(L);
    int id = (int)luaL_checkinteger(L, 1);
    entity_destroy(g_world->ecs, id);
    return 0;
}

/* entity_get_pos(entity_id) → x, y */
static int l_entity_get_pos(lua_State *L) {
    CHECK_WORLD(L);
    int id = (int)luaL_checkinteger(L, 1);
    PositionComponent *p = entity_pos(g_world->ecs, id);
    if (!p) { lua_pushnil(L); lua_pushnil(L); return 2; }
    lua_pushinteger(L, p->x);
    lua_pushinteger(L, p->y);
    return 2;
}

/* entity_set_pos(entity_id, x, y) */
static int l_entity_set_pos(lua_State *L) {
    CHECK_WORLD(L);
    int id = (int)luaL_checkinteger(L, 1);
    int x  = (int)luaL_checkinteger(L, 2);
    int y  = (int)luaL_checkinteger(L, 3);
    PositionComponent *p = entity_pos(g_world->ecs, id);
    if (p) { p->x = x; p->y = y; }
    return 0;
}

/* entity_get_hp(entity_id) → current, max */
static int l_entity_get_hp(lua_State *L) {
    CHECK_WORLD(L);
    int id = (int)luaL_checkinteger(L, 1);
    HealthComponent *h = entity_health(g_world->ecs, id);
    if (!h) { lua_pushinteger(L, 0); lua_pushinteger(L, 0); return 2; }
    lua_pushinteger(L, h->current);
    lua_pushinteger(L, h->max);
    return 2;
}

/* entity_damage(entity_id, amount) */
static int l_entity_damage(lua_State *L) {
    CHECK_WORLD(L);
    int id     = (int)luaL_checkinteger(L, 1);
    int amount = (int)luaL_checkinteger(L, 2);
    HealthComponent *h = entity_health(g_world->ecs, id);
    if (h) {
        h->current -= amount;
        if (h->current < 0) h->current = 0;
    }
    return 0;
}

/* entity_heal(entity_id, amount) */
static int l_entity_heal(lua_State *L) {
    CHECK_WORLD(L);
    int id     = (int)luaL_checkinteger(L, 1);
    int amount = (int)luaL_checkinteger(L, 2);
    HealthComponent *h = entity_health(g_world->ecs, id);
    if (h) {
        h->current += amount;
        if (h->current > h->max) h->current = h->max;
    }
    return 0;
}

/* entity_has_tag(entity_id, tag) → bool */
static int l_entity_has_tag(lua_State *L) {
    CHECK_WORLD(L);
    int id         = (int)luaL_checkinteger(L, 1);
    const char *tag = luaL_checkstring(L, 2);
    lua_pushboolean(L, entity_has_tag(g_world->ecs, id, tag));
    return 1;
}

/* entity_add_tag(entity_id, tag) */
static int l_entity_add_tag(lua_State *L) {
    CHECK_WORLD(L);
    int id         = (int)luaL_checkinteger(L, 1);
    const char *tag = luaL_checkstring(L, 2);
    entity_add_tag(g_world->ecs, id, tag);
    return 0;
}

/* entity_find_nearby(x, y, radius, tag) → {ids} */
static int l_entity_find_nearby(lua_State *L) {
    CHECK_WORLD(L);
    int x      = (int)luaL_checkinteger(L, 1);
    int y      = (int)luaL_checkinteger(L, 2);
    int radius = (int)luaL_checkinteger(L, 3);
    const char *tag = lua_isstring(L, 4) ? lua_tostring(L, 4) : NULL;

    int ids[256];
    int n = entities_nearby(g_world->ecs, x, y, radius, tag, ids, 256);

    lua_createtable(L, n, 0);
    for (int i = 0; i < n; i++) {
        lua_pushinteger(L, ids[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

/* =========================================================
 * === MAP ===
 * ========================================================= */

/* map_get_tile(x, y) → tile_type */
static int l_map_get_tile(lua_State *L) {
    CHECK_WORLD(L);
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    Tile *t = map_tile(&g_world->map, x, y);
    lua_pushinteger(L, t ? (int)t->type : -1);
    return 1;
}

/* map_set_tile(x, y, tile_type) */
static int l_map_set_tile(lua_State *L) {
    CHECK_WORLD(L);
    int x    = (int)luaL_checkinteger(L, 1);
    int y    = (int)luaL_checkinteger(L, 2);
    int type = (int)luaL_checkinteger(L, 3);
    Tile *t = map_tile(&g_world->map, x, y);
    if (t) t->type = (TileType)CLAMP(type, 0, TILE_COUNT - 1);
    return 0;
}

/* map_is_walkable(x, y) → bool */
static int l_map_is_walkable(lua_State *L) {
    CHECK_WORLD(L);
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, map_is_walkable(&g_world->map, x, y));
    return 1;
}

/* =========================================================
 * === WORLD ===
 * ========================================================= */

/* world_get_time() → hour, day, season */
static int l_world_get_time(lua_State *L) {
    CHECK_WORLD(L);
    lua_pushinteger(L, g_world->time.hour);
    lua_pushinteger(L, g_world->time.day);
    lua_pushinteger(L, g_world->time.season);
    return 3;
}

/* world_is_night() → bool */
static int l_world_is_night(lua_State *L) {
    CHECK_WORLD(L);
    lua_pushboolean(L, world_is_night(g_world));
    return 1;
}

/* world_get_weather() → weather_type */
static int l_world_get_weather(lua_State *L) {
    CHECK_WORLD(L);
    lua_pushinteger(L, (int)g_world->weather);
    return 1;
}

/* =========================================================
 * === UI ===
 * ========================================================= */
/* ui_log and ui_show_message are wired up via a callback
 * pointer set at engine init to avoid circular dependencies */
typedef void (*UILogFn)(const char *text, uint32_t color);
static UILogFn g_ui_log_fn = NULL;

void lua_api_set_ui_log(UILogFn fn) { g_ui_log_fn = fn; }

static int l_ui_log(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    uint32_t color   = lua_isnumber(L, 2) ?
        (uint32_t)lua_tointeger(L, 2) : COL_WHITE;
    if (g_ui_log_fn) g_ui_log_fn(text, color);
    return 0;
}

static int l_ui_show_message(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    if (g_ui_log_fn) g_ui_log_fn(text, COL_WHITE);
    return 0;
}

static int l_quest_start(lua_State *L) {
    const char *quest_id = luaL_checkstring(L, 1);
    int player_id = g_world && g_world->ecs ? g_world->ecs->player_id : -1;

    lua_pushboolean(L, quest_start(player_id, quest_id));
    return 1;
}

static int l_quest_active(lua_State *L) {
    const char *quest_id = luaL_checkstring(L, 1);

    lua_pushboolean(L, quest_active(quest_id));
    return 1;
}

static int l_player_level(lua_State *L) {
    CHECK_WORLD(L);
    if (g_world->ecs->player_id < 0) {
        lua_pushinteger(L, 0);
        return 1;
    }
    lua_pushinteger(L, g_world->ecs->stats[g_world->ecs->player_id].level);
    return 1;
}

static int l_faction_get_rep(lua_State *L) {
    const char *faction_id = luaL_checkstring(L, 1);
    int player_id = g_world && g_world->ecs ? g_world->ecs->player_id : -1;

    lua_pushinteger(L, faction_get_rep(player_id, faction_id));
    return 1;
}

static int l_faction_change_rep(lua_State *L) {
    const char *faction_id = luaL_checkstring(L, 1);
    int delta = (int)luaL_checkinteger(L, 2);
    int player_id = g_world && g_world->ecs ? g_world->ecs->player_id : -1;

    faction_change_rep(player_id, faction_id, delta);
    return 0;
}

static int l_npc_start_dialog(lua_State *L) {
    const char *npc_id = luaL_checkstring(L, 1);
    int player_id = g_world && g_world->ecs ? g_world->ecs->player_id : -1;

    lua_pushboolean(L, npc_start_dialog(player_id, npc_id));
    return 1;
}

static int l_quest_available(lua_State *L) {
    const char *quest_id = luaL_checkstring(L, 1);

    lua_pushboolean(L, quest_available(quest_id));
    return 1;
}

static int l_quest_advance(lua_State *L) {
    const char *quest_id = luaL_checkstring(L, 1);
    int player_id = g_world && g_world->ecs ? g_world->ecs->player_id : -1;

    lua_pushboolean(L, quest_advance(player_id, quest_id));
    return 1;
}

static int l_quest_fail(lua_State *L) {
    const char *quest_id = luaL_checkstring(L, 1);
    int player_id = g_world && g_world->ecs ? g_world->ecs->player_id : -1;

    lua_pushboolean(L, quest_fail(player_id, quest_id));
    return 1;
}

static int l_npc_dialog_open(lua_State *L) {
    lua_pushboolean(L, npc_dialog_open());
    return 1;
}

/* =========================================================
 * === REGISTRATION TABLE ===
 * ========================================================= */
static const luaL_Reg ASHLANDS_LIB[] = {
    /* Entities */
    { "entity_spawn",       l_entity_spawn       },
    { "entity_destroy",     l_entity_destroy     },
    { "entity_get_pos",     l_entity_get_pos     },
    { "entity_set_pos",     l_entity_set_pos     },
    { "entity_get_hp",      l_entity_get_hp      },
    { "entity_damage",      l_entity_damage      },
    { "entity_heal",        l_entity_heal        },
    { "entity_has_tag",     l_entity_has_tag     },
    { "entity_add_tag",     l_entity_add_tag     },
    { "entity_find_nearby", l_entity_find_nearby },
    /* Map */
    { "map_get_tile",       l_map_get_tile       },
    { "map_set_tile",       l_map_set_tile       },
    { "map_is_walkable",    l_map_is_walkable    },
    /* World */
    { "world_get_time",     l_world_get_time     },
    { "world_is_night",     l_world_is_night     },
    { "world_get_weather",  l_world_get_weather  },
    /* UI */
    { "ui_log",             l_ui_log             },
    { "ui_show_message",    l_ui_show_message    },
    /* Phase 3 systems */
    { "quest_start",        l_quest_start        },
    { "quest_active",       l_quest_active       },
    { "quest_available",    l_quest_available    },
    { "quest_advance",      l_quest_advance      },
    { "quest_fail",         l_quest_fail         },
    { "player_level",       l_player_level       },
    { "faction_get_rep",    l_faction_get_rep    },
    { "faction_change_rep", l_faction_change_rep },
    { "npc_start_dialog",   l_npc_start_dialog   },
    { "npc_dialog_open",    l_npc_dialog_open    },
    { NULL,                 NULL                 },
};

/* =========================================================
 * Public API
 * ========================================================= */

LuaContext *lua_ctx_create(void) {
    LuaContext *ctx = calloc(1, sizeof(LuaContext));
    if (!ctx) return NULL;

    ctx->L = luaL_newstate();
    if (!ctx->L) { free(ctx); return NULL; }

    luaL_openlibs(ctx->L);
    ctx->initialized = true;
    return ctx;
}

void lua_ctx_destroy(LuaContext *ctx) {
    if (!ctx) return;
    if (ctx->L) lua_close(ctx->L);
    free(ctx);
}

void lua_api_register(LuaContext *ctx) {
    if (!ctx || !ctx->L) return;
    lua_State *L = ctx->L;
    g_api_lua = L;

    /* Register all functions as globals */
    for (int i = 0; ASHLANDS_LIB[i].name; i++) {
        lua_pushcfunction(L, ASHLANDS_LIB[i].func);
        lua_setglobal(L, ASHLANDS_LIB[i].name);
    }

    texgen_register_lua(L);
    faction_register_lua(L);
    item_register_lua(L);
    npc_register_lua(L);
    quest_register_lua(L);

    /* Expose tile type constants */
    lua_pushinteger(L, TILE_FLOOR);       lua_setglobal(L, "TILE_FLOOR");
    lua_pushinteger(L, TILE_WALL);        lua_setglobal(L, "TILE_WALL");
    lua_pushinteger(L, TILE_WATER);       lua_setglobal(L, "TILE_WATER");
    lua_pushinteger(L, TILE_ASH);         lua_setglobal(L, "TILE_ASH");
    lua_pushinteger(L, TILE_STAIRS_DOWN); lua_setglobal(L, "TILE_STAIRS_DOWN");
    lua_pushinteger(L, TILE_STAIRS_UP);   lua_setglobal(L, "TILE_STAIRS_UP");
    lua_pushinteger(L, TILE_DOOR_SHUT);   lua_setglobal(L, "TILE_DOOR_SHUT");
    lua_pushinteger(L, TILE_DOOR_OPEN);   lua_setglobal(L, "TILE_DOOR_OPEN");

    /* Colour constants */
    lua_pushinteger(L, (lua_Integer)COL_WHITE);    lua_setglobal(L, "COL_WHITE");
    lua_pushinteger(L, (lua_Integer)COL_RED);      lua_setglobal(L, "COL_RED");
    lua_pushinteger(L, (lua_Integer)COL_GREEN);    lua_setglobal(L, "COL_GREEN");
    lua_pushinteger(L, (lua_Integer)COL_BLUE);     lua_setglobal(L, "COL_BLUE");
    lua_pushinteger(L, (lua_Integer)COL_YELLOW);   lua_setglobal(L, "COL_YELLOW");
    lua_pushinteger(L, (lua_Integer)COL_GRAY);     lua_setglobal(L, "COL_GRAY");
    lua_pushinteger(L, (lua_Integer)COL_BROWN);    lua_setglobal(L, "COL_BROWN");
}

bool lua_ctx_load_file(LuaContext *ctx, const char *path) {
    if (!ctx || !ctx->L) return false;
    if (luaL_dofile(ctx->L, path) != 0) {
        fprintf(stderr, "[Lua] Error loading %s: %s\n",
                path, lua_tostring(ctx->L, -1));
        lua_pop(ctx->L, 1);
        return false;
    }
    return true;
}

int lua_ctx_load_dir(LuaContext *ctx, const char *dir_path) {
    /* Use SDL_RWops-compatible approach: list via stdio glob fallback */
    /* For Phase 1, just return 0 — full dir scanning in Phase 2 */
    (void)ctx; (void)dir_path;
    return 0;
}

/* =========================================================
 * Event hooks
 * ========================================================= */
static bool call_hook(LuaContext *ctx, const char *fn, int n_args) {
    lua_State *L = ctx->L;
    lua_getglobal(L, fn);
    if (lua_isnil(L, -1)) { lua_pop(L, 1); return false; }
    /* args are already on stack below fn — rearrange */
    /* Simple: call with no args for now; entity IDs pushed as upvalues */
    if (lua_pcall(L, n_args, 0, 0) != 0) {
        fprintf(stderr, "[Lua] %s: %s\n", fn, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return true;
}

bool lua_call_on_turn(LuaContext *ctx, int entity_id) {
    lua_pushinteger(ctx->L, entity_id);
    return call_hook(ctx, "on_turn", 1);
}

bool lua_call_on_death(LuaContext *ctx, int entity_id) {
    lua_pushinteger(ctx->L, entity_id);
    return call_hook(ctx, "on_death", 1);
}

bool lua_call_on_see(LuaContext *ctx, int entity_id, int target_id) {
    lua_pushinteger(ctx->L, entity_id);
    lua_pushinteger(ctx->L, target_id);
    return call_hook(ctx, "on_see", 2);
}

bool lua_call_on_interact(LuaContext *ctx, int entity_id, int other_id) {
    lua_pushinteger(ctx->L, entity_id);
    lua_pushinteger(ctx->L, other_id);
    return call_hook(ctx, "on_interact", 2);
}

#endif
