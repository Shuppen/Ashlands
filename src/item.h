#ifndef ASHLANDS_ITEM_H
#define ASHLANDS_ITEM_H

#include <stdbool.h>
#include <stdint.h>

#include "ecs.h"

struct lua_State;

#define MAX_ITEM_DEFS 128

typedef struct {
    char id[ITEM_NAME_LEN];
    char name[ITEM_NAME_LEN];
    int type;
    int value;
    int weight;
    int stack_size;
    int attack_bonus;
    char glyph;
    uint32_t color;
    char texture_id[ITEM_NAME_LEN];
    int use_effect_ref;
} ItemDef;

void item_registry_init(void);
void item_registry_shutdown(void);
int item_register_lua(struct lua_State *L);

bool item_register(const ItemDef *def);
const ItemDef *item_get(const char *id);
int item_count(void);
const ItemDef *item_get_by_index(int index);
bool item_exists(const char *id);

int item_type_from_name(const char *name);
int item_spawn(World *w, const char *item_id, int x, int y,
               bool into_inventory, int inventory_slot, int stack_count);
bool item_use(World *w, int user_id, int slot_index);

#endif
