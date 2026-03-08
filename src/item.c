#include "item.h"

#include "lua_api.h"

#include "lua_compat.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    ItemDef defs[MAX_ITEM_DEFS];
    int count;
} ItemRegistry;

static ItemRegistry s_items;

static int l_register_item(struct lua_State *L) {
    ItemDef def;
    int index;
    const char *type_name;

    memset(&def, 0, sizeof(def));
    luaL_checktype(L, 1, LUA_TTABLE);
    index = lua_absindex(L, 1);

    lua_getfield(L, index, "id");
    strncpy(def.id, luaL_checkstring(L, -1), ITEM_NAME_LEN - 1);
    lua_pop(L, 1);
    lua_getfield(L, index, "name");
    strncpy(def.name, luaL_optstring(L, -1, def.id), ITEM_NAME_LEN - 1);
    lua_pop(L, 1);
    lua_getfield(L, index, "type");
    type_name = luaL_optstring(L, -1, "material");
    def.type = item_type_from_name(type_name);
    lua_pop(L, 1);
    lua_getfield(L, index, "value");
    def.value = (int)luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);
    lua_getfield(L, index, "weight");
    def.weight = (int)luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);
    lua_getfield(L, index, "stack_size");
    def.stack_size = (int)luaL_optinteger(L, -1, 1);
    lua_pop(L, 1);
    lua_getfield(L, index, "attack_bonus");
    def.attack_bonus = (int)luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);
    lua_getfield(L, index, "render");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "ascii");
        if (lua_istable(L, -1)) {
            const char *glyph;

            lua_getfield(L, -1, "glyph");
            glyph = luaL_optstring(L, -1, "!");
            def.glyph = glyph[0];
            lua_pop(L, 1);
            lua_getfield(L, -1, "color");
            def.color = (uint32_t)luaL_optinteger(L, -1, COL_YELLOW);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
        lua_getfield(L, -1, "tile_2d");
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "texture");
            strncpy(def.texture_id, luaL_optstring(L, -1, ""), ITEM_NAME_LEN - 1);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_getfield(L, index, "use_effect");
    def.use_effect_ref = lua_api_ref_function(L, -1);
    lua_pop(L, 1);

    lua_pushboolean(L, item_register(&def));
    return 1;
}

void item_registry_init(void) {
    memset(&s_items, 0, sizeof(s_items));
}

void item_registry_shutdown(void) {
    memset(&s_items, 0, sizeof(s_items));
}

int item_register_lua(struct lua_State *L) {
    lua_pushcfunction(L, l_register_item);
    lua_setglobal(L, "register_item");
    return 0;
}

bool item_register(const ItemDef *def) {
    int slot = -1;

    if (!def || !def->id[0]) {
        return false;
    }

    for (int i = 0; i < s_items.count; i++) {
        if (strcmp(s_items.defs[i].id, def->id) == 0) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        if (s_items.count >= MAX_ITEM_DEFS) {
            return false;
        }
        slot = s_items.count++;
    }

    s_items.defs[slot] = *def;
    return true;
}

const ItemDef *item_get(const char *id) {
    if (!id) {
        return NULL;
    }
    for (int i = 0; i < s_items.count; i++) {
        if (strcmp(s_items.defs[i].id, id) == 0) {
            return &s_items.defs[i];
        }
    }
    return NULL;
}

bool item_exists(const char *id) {
    return item_get(id) != NULL;
}

int item_count(void) {
    return s_items.count;
}

const ItemDef *item_get_by_index(int index) {
    if (index < 0 || index >= s_items.count) {
        return NULL;
    }
    return &s_items.defs[index];
}

int item_type_from_name(const char *name) {
    if (!name) {
        return 0;
    }
    if (strcmp(name, "food") == 0) return 1;
    if (strcmp(name, "material") == 0) return 2;
    if (strcmp(name, "weapon") == 0) return 3;
    if (strcmp(name, "consumable") == 0) return 4;
    return 0;
}

int item_spawn(World *w, const char *item_id, int x, int y,
               bool into_inventory, int inventory_slot, int stack_count) {
    const ItemDef *def;
    int item_ent;
    ItemComponent *item;
    RenderComponent *rc;

    if (!w || !item_id) {
        return -1;
    }

    def = item_get(item_id);
    if (!def) {
        return -1;
    }

    item_ent = entity_create(w);
    if (item_ent < 0) {
        return -1;
    }

    entity_add_comp(w, item_ent, COMP_ITEM | COMP_RENDER);
    if (!into_inventory) {
        entity_add_comp(w, item_ent, COMP_POSITION);
        w->positions[item_ent].x = x;
        w->positions[item_ent].y = y;
        w->positions[item_ent].z = 0;
        entity_add_tag(w, item_ent, "item");
    }

    item = entity_item(w, item_ent);
    rc = entity_render(w, item_ent);
    if (!item || !rc) {
        entity_destroy(w, item_ent);
        return -1;
    }

    snprintf(item->id, sizeof(item->id), "%s", def->id);
    snprintf(item->name, sizeof(item->name), "%s", def->name);
    item->type = def->type;
    item->value = def->value;
    item->weight = def->weight;
    item->stack_size = MAX(def->stack_size, 1);
    item->stack_count = MAX(stack_count, 1);

    rc->glyph = def->glyph ? def->glyph : '!';
    rc->fg_color = def->color ? def->color : COL_YELLOW;
    rc->bg_color = COL_BLACK;
    rc->sprite_id = -1;
    snprintf(rc->texture_id, sizeof(rc->texture_id), "%s", def->texture_id);
    rc->mesh_id[0] = '\0';

    if (into_inventory) {
        InventoryComponent *inv = entity_inventory(w, w->player_id);
        if (!inv || inventory_slot < 0 || inventory_slot >= MAX_INVENTORY_SLOTS) {
            entity_destroy(w, item_ent);
            return -1;
        }
        inv->slots[inventory_slot] = item_ent;
    }

    return item_ent;
}

bool item_use(World *w, int user_id, int slot_index) {
    InventoryComponent *inv;
    int item_id;
    ItemComponent *item;
    const ItemDef *def;

    if (!w || user_id < 0) {
        return false;
    }

    inv = entity_inventory(w, user_id);
    if (!inv || slot_index < 0 || slot_index >= MAX_INVENTORY_SLOTS) {
        return false;
    }

    item_id = inv->slots[slot_index];
    if (item_id < 0 || !entity_is_alive(w, item_id)) {
        return false;
    }

    item = entity_item(w, item_id);
    if (!item) {
        return false;
    }

    def = item_get(item->id);
    if (def && def->use_effect_ref != LUA_NOREF) {
        lua_api_call_ref_int(def->use_effect_ref, user_id);
    }

    item->stack_count = MAX(item->stack_count - 1, 0);
    if (item->stack_count <= 0) {
        entity_destroy(w, item_id);
        inv->slots[slot_index] = -1;
        inv->count = MAX(inv->count - 1, 0);
    }
    return true;
}
