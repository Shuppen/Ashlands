/*
 * lua_api.h — LuaJIT bindings for the Ashlands engine
 * C11, MIT License
 *
 * Exposes the engine C API to Lua scripts so that "vibe coders"
 * can define mobs, items, biomes, quests, textures, and more.
 */
#ifndef ASHLANDS_LUA_API_H
#define ASHLANDS_LUA_API_H

#include <stdbool.h>

/* Forward declaration — avoids including lua headers everywhere */
struct lua_State;

typedef struct LuaContext {
    struct lua_State *L;
    bool initialized;
} LuaContext;

/* =========================================================
 * Lifecycle
 * ========================================================= */
LuaContext *lua_ctx_create(void);
void        lua_ctx_destroy(LuaContext *ctx);

/* Load and execute a Lua file (returns false on error, prints msg) */
bool lua_ctx_load_file(LuaContext *ctx, const char *path);

/* Load all .lua files from a directory (non-recursive) */
int lua_ctx_load_dir(LuaContext *ctx, const char *dir_path);

/* Set the global WorldState pointer used by callbacks */
void lua_ctx_set_world(LuaContext *ctx, void *world_state);

/* =========================================================
 * Register all C functions into Lua
 * Called once after lua_ctx_create()
 * ========================================================= */
void lua_api_register(LuaContext *ctx);

/* =========================================================
 * Call Lua hooks on game events
 * Returns true if the hook exists and was called
 * ========================================================= */
bool lua_call_on_turn(LuaContext *ctx, int entity_id);
bool lua_call_on_death(LuaContext *ctx, int entity_id);
bool lua_call_on_see(LuaContext *ctx, int entity_id, int target_id);
bool lua_call_on_interact(LuaContext *ctx, int entity_id, int other_id);

#endif /* ASHLANDS_LUA_API_H */
