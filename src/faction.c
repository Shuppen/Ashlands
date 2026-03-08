#include "faction.h"

#include "world.h"

#include "lua_compat.h"

#include <string.h>

typedef struct {
    FactionDef defs[MAX_FACTIONS];
    float relations[MAX_FACTIONS][MAX_FACTIONS];
    int player_rep[MAX_FACTIONS];
    int count;
    WorldState *world;
} FactionState;

static FactionState s_factions;

static int faction_parse_relations(struct lua_State *L, int index, int self_index) {
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        const char *other_id = lua_tostring(L, -2);
        float value = (float)luaL_optnumber(L, -1, 0.0);
        int other_index = faction_find(other_id);

        if (other_index >= 0) {
            s_factions.relations[self_index][other_index] = value;
            s_factions.relations[other_index][self_index] = value;
        }
        lua_pop(L, 1);
    }

    return 0;
}

static int l_register_faction(struct lua_State *L) {
    FactionDef def;
    int index;
    int faction_index;

    memset(&def, 0, sizeof(def));
    luaL_checktype(L, 1, LUA_TTABLE);
    index = lua_absindex(L, 1);

    lua_getfield(L, index, "id");
    strncpy(def.id, luaL_checkstring(L, -1), MAX_FACTION_NAME - 1);
    lua_pop(L, 1);
    lua_getfield(L, index, "name");
    strncpy(def.name, luaL_optstring(L, -1, def.id), MAX_FACTION_NAME - 1);
    lua_pop(L, 1);
    lua_getfield(L, index, "default_player_rep");
    def.default_player_rep = (int)luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);
    lua_getfield(L, index, "trader_bias");
    def.trader_bias = (int)luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);

    lua_getfield(L, index, "territory_biomes");
    if (lua_istable(L, -1)) {
        int count = (int)lua_objlen(L, -1);
        def.territory_count = MIN(count, MAX_FACTION_BIOMES);
        for (int i = 0; i < def.territory_count; i++) {
            lua_rawgeti(L, -1, i + 1);
            strncpy(def.territory_biomes[i], luaL_optstring(L, -1, ""),
                    MAX_FACTION_NAME - 1);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    faction_index = faction_register(&def);
    if (faction_index >= 0) {
        lua_getfield(L, index, "relations");
        if (lua_istable(L, -1)) {
            faction_parse_relations(L, lua_gettop(L), faction_index);
        }
        lua_pop(L, 1);
    }

    lua_pushboolean(L, faction_index >= 0);
    return 1;
}

void faction_init(void) {
    memset(&s_factions, 0, sizeof(s_factions));
}

void faction_shutdown(void) {
    memset(&s_factions, 0, sizeof(s_factions));
}

void faction_set_world(WorldState *ws) {
    s_factions.world = ws;
}

int faction_register(const FactionDef *def) {
    int index;

    if (!def || !def->id[0]) {
        return -1;
    }

    index = faction_find(def->id);
    if (index < 0) {
        if (s_factions.count >= MAX_FACTIONS) {
            return -1;
        }
        index = s_factions.count++;
    }

    s_factions.defs[index] = *def;
    s_factions.player_rep[index] = def->default_player_rep;
    s_factions.relations[index][index] = 1.0f;
    return index;
}

int faction_find(const char *id) {
    if (!id) {
        return -1;
    }

    for (int i = 0; i < s_factions.count; i++) {
        if (strcmp(s_factions.defs[i].id, id) == 0) {
            return i;
        }
    }

    return -1;
}

bool faction_exists(const char *id) {
    return faction_find(id) >= 0;
}

int faction_count(void) {
    return s_factions.count;
}

const FactionDef *faction_get_by_index(int index) {
    if (index < 0 || index >= s_factions.count) {
        return NULL;
    }
    return &s_factions.defs[index];
}

int faction_register_lua(struct lua_State *L) {
    lua_pushcfunction(L, l_register_faction);
    lua_setglobal(L, "register_faction");
    return 0;
}

int faction_get_rep(int player_id, const char *faction_id) {
    int index = faction_find(faction_id);

    (void)player_id;
    return index >= 0 ? s_factions.player_rep[index] : 0;
}

void faction_set_rep(int player_id, const char *faction_id, int value) {
    int index = faction_find(faction_id);

    (void)player_id;
    if (index >= 0) {
        s_factions.player_rep[index] = CLAMP(value, -100, 100);
    }
}

void faction_change_rep(int player_id, const char *faction_id, int delta) {
    int index = faction_find(faction_id);

    (void)player_id;
    if (index >= 0) {
        s_factions.player_rep[index] = CLAMP(s_factions.player_rep[index] + delta,
                                             -100, 100);
    }
}

float faction_get_relation(const char *faction_a, const char *faction_b) {
    int a = faction_find(faction_a);
    int b = faction_find(faction_b);

    if (a < 0 || b < 0) {
        return 0.0f;
    }

    return s_factions.relations[a][b];
}

bool faction_is_hostile(int entity_id, int target_id) {
    FactionComponent *fa;
    FactionComponent *fb;
    World *ecs;
    float relation;

    if (!s_factions.world) {
        return false;
    }

    ecs = s_factions.world->ecs;
    if (target_id == ecs->player_id) {
        fa = entity_faction(ecs, entity_id);
        if (!fa || fa->faction_id < 0 || fa->faction_id >= s_factions.count) {
            return false;
        }
        return faction_get_rep(target_id, s_factions.defs[fa->faction_id].id) < -20 ||
               fa->hostility > 50;
    }

    fa = entity_faction(ecs, entity_id);
    fb = entity_faction(ecs, target_id);
    if (!fa || !fb) {
        return false;
    }
    if (fa->faction_id < 0 || fb->faction_id < 0 ||
        fa->faction_id >= s_factions.count || fb->faction_id >= s_factions.count) {
        return false;
    }

    relation = s_factions.relations[fa->faction_id][fb->faction_id];
    return relation < -0.25f || fa->hostility > 50;
}
