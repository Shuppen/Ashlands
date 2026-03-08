#ifndef ASHLANDS_FACTION_H
#define ASHLANDS_FACTION_H

#include <stdbool.h>

struct lua_State;
typedef struct WorldState WorldState;

#define MAX_FACTIONS 16
#define MAX_FACTION_NAME 64
#define MAX_FACTION_BIOMES 8

typedef struct {
    char id[MAX_FACTION_NAME];
    char name[MAX_FACTION_NAME];
    char territory_biomes[MAX_FACTION_BIOMES][MAX_FACTION_NAME];
    int territory_count;
    int default_player_rep;
    int trader_bias;
} FactionDef;

void faction_init(void);
void faction_shutdown(void);
void faction_set_world(WorldState *ws);

int faction_register(const FactionDef *def);
int faction_find(const char *id);
bool faction_exists(const char *id);
int faction_count(void);
const FactionDef *faction_get_by_index(int index);

int faction_register_lua(struct lua_State *L);

int faction_get_rep(int player_id, const char *faction_id);
void faction_set_rep(int player_id, const char *faction_id, int value);
void faction_change_rep(int player_id, const char *faction_id, int delta);
float faction_get_relation(const char *faction_a, const char *faction_b);
bool faction_is_hostile(int entity_id, int target_id);

#endif
