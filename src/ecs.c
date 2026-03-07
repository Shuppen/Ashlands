/*
 * ecs.c — Entity-Component-System implementation
 * C11, MIT License
 */
#include "ecs.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* =========================================================
 * World lifecycle
 * ========================================================= */

World *world_create(void) {
    World *w = calloc(1, sizeof(World));
    if (!w) return NULL;
    world_clear(w);
    return w;
}

void world_destroy(World *w) {
    if (w) free(w);
}

void world_clear(World *w) {
    memset(w->entities, 0, sizeof(w->entities));
    w->entity_count = 0;
    w->free_count   = 0;
    w->player_id    = -1;

    /* Pre-populate free list in reverse so ID 0 is given out first */
    for (int i = MAX_ENTITIES - 1; i >= 0; i--) {
        w->free_ids[w->free_count++] = i;
    }
}

/* =========================================================
 * Entity CRUD
 * ========================================================= */

int entity_create(World *w) {
    if (w->free_count == 0) {
        fprintf(stderr, "[ECS] entity limit reached (%d)\n", MAX_ENTITIES);
        return -1;
    }
    int id = w->free_ids[--w->free_count];
    Entity *e = &w->entities[id];
    memset(e, 0, sizeof(Entity));
    e->id      = (uint32_t)id;
    e->mask    = 0;
    e->active  = true;
    e->tag_count = 0;
    w->entity_count++;
    return id;
}

void entity_destroy(World *w, int id) {
    if (id < 0 || id >= MAX_ENTITIES) return;
    Entity *e = &w->entities[id];
    if (!e->active) return;
    e->active = false;
    e->mask   = 0;
    w->free_ids[w->free_count++] = id;
    w->entity_count--;
}

bool entity_is_alive(const World *w, int id) {
    if (id < 0 || id >= MAX_ENTITIES) return false;
    return w->entities[id].active;
}

/* =========================================================
 * Component access
 * ========================================================= */

bool entity_has(const World *w, int id, uint64_t comp_mask) {
    if (!entity_is_alive(w, id)) return false;
    return (w->entities[id].mask & comp_mask) == comp_mask;
}

void entity_add_comp(World *w, int id, uint64_t comp_mask) {
    if (!entity_is_alive(w, id)) return;
    w->entities[id].mask |= comp_mask;
}

void entity_rem_comp(World *w, int id, uint64_t comp_mask) {
    if (!entity_is_alive(w, id)) return;
    w->entities[id].mask &= ~comp_mask;
}

/* Typed shortcuts — return pointer into flat arrays */

PositionComponent *entity_pos(World *w, int id) {
    if (!entity_has(w, id, COMP_POSITION)) return NULL;
    return &w->positions[id];
}

HealthComponent *entity_health(World *w, int id) {
    if (!entity_has(w, id, COMP_HEALTH)) return NULL;
    return &w->healths[id];
}

RenderComponent *entity_render(World *w, int id) {
    if (!entity_has(w, id, COMP_RENDER)) return NULL;
    return &w->renders[id];
}

StatsComponent *entity_stats(World *w, int id) {
    if (!entity_has(w, id, COMP_STATS)) return NULL;
    return &w->stats[id];
}

ScriptComponent *entity_script(World *w, int id) {
    if (!entity_has(w, id, COMP_SCRIPT)) return NULL;
    return &w->scripts[id];
}

InventoryComponent *entity_inventory(World *w, int id) {
    if (!entity_has(w, id, COMP_INVENTORY)) return NULL;
    return &w->inventories[id];
}

FactionComponent *entity_faction(World *w, int id) {
    if (!entity_has(w, id, COMP_FACTION)) return NULL;
    return &w->factions[id];
}

AIComponent *entity_ai(World *w, int id) {
    if (!entity_has(w, id, COMP_AI)) return NULL;
    return &w->ais[id];
}

PhysicsComponent *entity_physics(World *w, int id) {
    if (!entity_has(w, id, COMP_PHYSICS)) return NULL;
    return &w->physics[id];
}

ItemComponent *entity_item(World *w, int id) {
    if (!entity_has(w, id, COMP_ITEM)) return NULL;
    return &w->items[id];
}

/* =========================================================
 * Tag helpers
 * ========================================================= */

bool entity_has_tag(const World *w, int id, const char *tag) {
    if (!entity_is_alive(w, id)) return false;
    const Entity *e = &w->entities[id];
    for (int i = 0; i < e->tag_count; i++) {
        if (strncmp(e->tags[i], tag, TAG_NAME_LEN) == 0) return true;
    }
    return false;
}

void entity_add_tag(World *w, int id, const char *tag) {
    if (!entity_is_alive(w, id)) return;
    if (entity_has_tag(w, id, tag)) return;   /* already present */
    Entity *e = &w->entities[id];
    if (e->tag_count >= MAX_ENTITY_TAGS) {
        fprintf(stderr, "[ECS] tag limit for entity %d\n", id);
        return;
    }
    strncpy(e->tags[e->tag_count++], tag, TAG_NAME_LEN - 1);
}

void entity_rem_tag(World *w, int id, const char *tag) {
    if (!entity_is_alive(w, id)) return;
    Entity *e = &w->entities[id];
    for (int i = 0; i < e->tag_count; i++) {
        if (strncmp(e->tags[i], tag, TAG_NAME_LEN) == 0) {
            /* Swap with last */
            e->tag_count--;
            if (i < e->tag_count) {
                memcpy(e->tags[i], e->tags[e->tag_count], TAG_NAME_LEN);
            }
            return;
        }
    }
}

/* =========================================================
 * Spatial query
 * ========================================================= */

int entities_nearby(const World *w, int x, int y, int radius,
                    const char *tag, int *out, int max_out)
{
    int found = 0;
    for (int id = 0; id < MAX_ENTITIES && found < max_out; id++) {
        if (!w->entities[id].active) continue;
        if (!entity_has((World*)w, id, COMP_POSITION)) continue;
        const PositionComponent *p = &w->positions[id];
        int dx = p->x - x;
        int dy = p->y - y;
        if (dx*dx + dy*dy <= radius*radius) {
            if (!tag || entity_has_tag(w, id, tag)) {
                out[found++] = id;
            }
        }
    }
    return found;
}
