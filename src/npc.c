#include "npc.h"

#include "ecs.h"
#include "faction.h"
#include "lua_api.h"
#include "quest.h"

#include "lua_compat.h"

#include <string.h>

typedef struct {
    NpcDef defs[MAX_NPCS];
    int entity_ids[MAX_NPCS];
    int count;
    int active_npc;
    int active_node;
    bool dialog_open;
} NpcState;

static NpcState s_npcs;

static int npc_find_index(const char *npc_id) {
    for (int i = 0; i < s_npcs.count; i++) {
        if (strcmp(s_npcs.defs[i].id, npc_id) == 0) {
            return i;
        }
    }
    return -1;
}

static int dialog_find_node(const NpcDef *def, const char *node_id) {
    for (int i = 0; i < def->node_count; i++) {
        if (strcmp(def->nodes[i].id, node_id) == 0) {
            return i;
        }
    }
    return -1;
}

static int l_register_npc(struct lua_State *L) {
    NpcDef def;
    int index;

    memset(&def, 0, sizeof(def));
    luaL_checktype(L, 1, LUA_TTABLE);
    index = lua_absindex(L, 1);

    lua_getfield(L, index, "id");
    strncpy(def.id, luaL_checkstring(L, -1), NPC_ID_LEN - 1);
    lua_pop(L, 1);
    lua_getfield(L, index, "name");
    strncpy(def.name, luaL_optstring(L, -1, def.id), NPC_ID_LEN - 1);
    lua_pop(L, 1);
    lua_getfield(L, index, "faction");
    strncpy(def.faction, luaL_optstring(L, -1, ""), NPC_ID_LEN - 1);
    lua_pop(L, 1);

    lua_getfield(L, index, "render");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "ascii");
        if (lua_istable(L, -1)) {
            const char *glyph;
            lua_getfield(L, -1, "glyph");
            glyph = luaL_optstring(L, -1, "N");
            def.ascii_glyph = glyph[0];
            lua_pop(L, 1);
            lua_getfield(L, -1, "color");
            def.ascii_color = (uint32_t)luaL_optinteger(L, -1, COL_WHITE);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "dialog");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0 && def.node_count < MAX_DIALOG_NODES) {
            DialogNode *node = &def.nodes[def.node_count++];
            int node_idx = lua_gettop(L);
            const char *node_id = lua_tostring(L, -2);

            strncpy(node->id, node_id ? node_id : "start", NPC_ID_LEN - 1);
            lua_getfield(L, node_idx, "text");
            strncpy(node->text, luaL_optstring(L, -1, ""), sizeof(node->text) - 1);
            lua_pop(L, 1);
            lua_getfield(L, node_idx, "options");
            if (lua_istable(L, -1)) {
                int count = (int)lua_objlen(L, -1);
                node->option_count = MIN(count, MAX_DIALOG_OPTIONS);
                for (int i = 0; i < node->option_count; i++) {
                    DialogOption *opt = &node->options[i];

                    lua_rawgeti(L, -1, i + 1);
                    lua_getfield(L, -1, "text");
                    strncpy(opt->text, luaL_optstring(L, -1, "[Continue]"),
                            sizeof(opt->text) - 1);
                    lua_pop(L, 1);
                    lua_getfield(L, -1, "next");
                    if (!lua_isnil(L, -1)) {
                        strncpy(opt->next, luaL_optstring(L, -1, ""), NPC_ID_LEN - 1);
                    }
                    lua_pop(L, 1);
                    lua_getfield(L, -1, "condition");
                    opt->condition_ref = lua_api_ref_function(L, -1);
                    lua_pop(L, 1);
                    lua_getfield(L, -1, "effect");
                    opt->effect_ref = lua_api_ref_function(L, -1);
                    lua_pop(L, 1);
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 2);
        }
    }
    lua_pop(L, 1);

    lua_pushboolean(L, npc_register(&def) >= 0);
    return 1;
}

void npc_init(void) {
    memset(&s_npcs, 0, sizeof(s_npcs));
    for (int i = 0; i < MAX_NPCS; i++) {
        s_npcs.entity_ids[i] = -1;
    }
    s_npcs.active_npc = -1;
    s_npcs.active_node = -1;
}

void npc_shutdown(void) {
    memset(&s_npcs, 0, sizeof(s_npcs));
}

int npc_register(const NpcDef *def) {
    int index;

    if (!def || !def->id[0]) {
        return -1;
    }

    index = npc_find_index(def->id);
    if (index < 0) {
        if (s_npcs.count >= MAX_NPCS) {
            return -1;
        }
        index = s_npcs.count++;
        s_npcs.entity_ids[index] = -1;
    }

    s_npcs.defs[index] = *def;
    return index;
}

int npc_register_lua(struct lua_State *L) {
    lua_pushcfunction(L, l_register_npc);
    lua_setglobal(L, "register_npc");
    return 0;
}

int npc_spawn(World *w, const char *npc_id, int x, int y) {
    int npc_index = npc_find_index(npc_id);
    int entity_id;
    RenderComponent *rc;
    int faction_index;

    if (!w || npc_index < 0) {
        return -1;
    }

    entity_id = entity_create(w);
    if (entity_id < 0) {
        return -1;
    }

    entity_add_comp(w, entity_id, COMP_POSITION | COMP_RENDER | COMP_FACTION);
    w->positions[entity_id].x = x;
    w->positions[entity_id].y = y;
    w->positions[entity_id].z = 0;
    rc = &w->renders[entity_id];
    rc->glyph = s_npcs.defs[npc_index].ascii_glyph ? s_npcs.defs[npc_index].ascii_glyph : 'N';
    rc->fg_color = s_npcs.defs[npc_index].ascii_color ? s_npcs.defs[npc_index].ascii_color : COL_YELLOW;
    rc->bg_color = COL_BLACK;
    rc->sprite_id = -1;
    rc->texture_id[0] = '\0';
    rc->mesh_id[0] = '\0';
    faction_index = faction_find(s_npcs.defs[npc_index].faction);
    w->factions[entity_id].faction_id = faction_index >= 0 ? faction_index : 0;
    s_npcs.entity_ids[npc_index] = entity_id;
    entity_add_tag(w, entity_id, npc_id);
    entity_add_tag(w, entity_id, "npc");
    return entity_id;
}

bool npc_start_dialog(int player_id, const char *npc_id) {
    int index;

    (void)player_id;
    index = npc_find_index(npc_id);
    if (index < 0) {
        return false;
    }

    s_npcs.active_npc = index;
    s_npcs.active_node = dialog_find_node(&s_npcs.defs[index], "start");
    if (s_npcs.active_node < 0) {
        s_npcs.active_node = 0;
    }
    s_npcs.dialog_open = true;
    return true;
}

bool npc_select_option(int player_id, int option_index) {
    const DialogNode *node;
    int next_node;

    (void)player_id;
    node = dialog_get_current(player_id);
    int actual_index;

    if (!node || option_index < 0) {
        return false;
    }

    actual_index = -1;
    for (int i = 0, visible = 0; i < node->option_count; i++) {
        if (node->options[i].condition_ref != LUA_NOREF &&
            !lua_api_call_ref_bool(node->options[i].condition_ref)) {
            continue;
        }
        if (visible == option_index) {
            actual_index = i;
            break;
        }
        visible++;
    }
    if (actual_index < 0) {
        return false;
    }

    if (node->options[actual_index].effect_ref != LUA_NOREF) {
        lua_api_call_ref_void(node->options[actual_index].effect_ref);
    }
    quest_on_talk_to(s_npcs.defs[s_npcs.active_npc].id);

    if (!node->options[actual_index].next[0]) {
        s_npcs.dialog_open = false;
        return true;
    }

    next_node = dialog_find_node(&s_npcs.defs[s_npcs.active_npc],
                                 node->options[actual_index].next);
    if (next_node < 0) {
        s_npcs.dialog_open = false;
        return false;
    }

    s_npcs.active_node = next_node;
    return true;
}

bool npc_dialog_open(void) {
    return s_npcs.dialog_open;
}

void npc_close_dialog(void) {
    s_npcs.dialog_open = false;
}

int npc_get_visible_options(int player_id, DialogOption *out_options, int max_out) {
    const DialogNode *node = dialog_get_current(player_id);
    int count = 0;

    if (!node || !out_options || max_out <= 0) {
        return 0;
    }

    for (int i = 0; i < node->option_count && count < max_out; i++) {
        if (node->options[i].condition_ref != LUA_NOREF &&
            !lua_api_call_ref_bool(node->options[i].condition_ref)) {
            continue;
        }
        out_options[count++] = node->options[i];
    }
    return count;
}

const DialogNode *dialog_get_current(int player_id) {
    (void)player_id;
    if (!s_npcs.dialog_open || s_npcs.active_npc < 0 || s_npcs.active_node < 0) {
        return NULL;
    }
    return &s_npcs.defs[s_npcs.active_npc].nodes[s_npcs.active_node];
}

const NpcDef *npc_find_def(const char *npc_id) {
    int index = npc_find_index(npc_id);
    return index >= 0 ? &s_npcs.defs[index] : NULL;
}

const char *npc_get_active_id(void) {
    if (s_npcs.active_npc < 0) {
        return NULL;
    }
    return s_npcs.defs[s_npcs.active_npc].id;
}

const char *npc_get_active_node_id(void) {
    if (s_npcs.active_npc < 0 || s_npcs.active_node < 0) {
        return NULL;
    }
    return s_npcs.defs[s_npcs.active_npc].nodes[s_npcs.active_node].id;
}

bool npc_restore_dialog(const char *npc_id, const char *node_id, bool open) {
    int npc_index = npc_find_index(npc_id);
    int node_index;

    if (npc_index < 0) {
        s_npcs.dialog_open = false;
        return false;
    }

    node_index = node_id ? dialog_find_node(&s_npcs.defs[npc_index], node_id) : -1;
    s_npcs.active_npc = npc_index;
    s_npcs.active_node = node_index >= 0 ? node_index : 0;
    s_npcs.dialog_open = open;
    return true;
}

const char *npc_id_for_entity(int entity_id) {
    for (int i = 0; i < s_npcs.count; i++) {
        if (s_npcs.entity_ids[i] == entity_id) {
            return s_npcs.defs[i].id;
        }
    }
    return NULL;
}

int npc_entity_for_id(const char *npc_id) {
    int index = npc_find_index(npc_id);
    if (index < 0) {
        return -1;
    }
    return s_npcs.entity_ids[index];
}
