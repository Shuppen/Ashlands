/*
 * ecs.h — Entity-Component-System interface
 * C11, MIT License
 */
#ifndef ASHLANDS_ECS_H
#define ASHLANDS_ECS_H

#include "../include/ashlands.h"

/* =========================================================
 * World — holds all entity data in flat arrays (cache-friendly)
 * ========================================================= */
typedef struct {
    /* Entity bookkeeping */
    Entity            entities[MAX_ENTITIES];
    int               entity_count;
    int               free_ids[MAX_ENTITIES];
    int               free_count;

    /* Component arrays — indexed by entity ID */
    PositionComponent positions[MAX_ENTITIES];
    HealthComponent   healths[MAX_ENTITIES];
    RenderComponent   renders[MAX_ENTITIES];
    StatsComponent    stats[MAX_ENTITIES];
    ScriptComponent   scripts[MAX_ENTITIES];
    InventoryComponent inventories[MAX_ENTITIES];
    FactionComponent  factions[MAX_ENTITIES];
    AIComponent       ais[MAX_ENTITIES];
    PhysicsComponent  physics[MAX_ENTITIES];
    ItemComponent     items[MAX_ENTITIES];

    /* Player entity ID (special-cased for fast access) */
    int player_id;
} World;

/* =========================================================
 * World lifecycle
 * ========================================================= */
World *world_create(void);
void   world_destroy(World *w);
void   world_clear(World *w);

/* =========================================================
 * Entity CRUD
 * ========================================================= */
int   entity_create(World *w);                    /* returns entity ID */
void  entity_destroy(World *w, int id);
bool  entity_is_alive(const World *w, int id);

/* =========================================================
 * Component access
 * ========================================================= */
bool entity_has(const World *w, int id, uint64_t comp_mask);
void entity_add_comp(World *w, int id, uint64_t comp_mask);
void entity_rem_comp(World *w, int id, uint64_t comp_mask);

/* Typed shortcuts */
PositionComponent  *entity_pos(World *w, int id);
HealthComponent    *entity_health(World *w, int id);
RenderComponent    *entity_render(World *w, int id);
StatsComponent     *entity_stats(World *w, int id);
ScriptComponent    *entity_script(World *w, int id);
InventoryComponent *entity_inventory(World *w, int id);
FactionComponent   *entity_faction(World *w, int id);
AIComponent        *entity_ai(World *w, int id);
PhysicsComponent   *entity_physics(World *w, int id);
ItemComponent      *entity_item(World *w, int id);

/* =========================================================
 * Tag helpers
 * ========================================================= */
bool entity_has_tag(const World *w, int id, const char *tag);
void entity_add_tag(World *w, int id, const char *tag);
void entity_rem_tag(World *w, int id, const char *tag);
void entity_clear_tags(World *w, int id);

/* =========================================================
 * Spatial query
 * ========================================================= */
/* Fills out[] with entity IDs near (x,y) with given tag (NULL = any).
 * Returns count (capped at max_out). */
int entities_nearby(const World *w, int x, int y, int radius,
                    const char *tag, int *out, int max_out);

#endif /* ASHLANDS_ECS_H */
