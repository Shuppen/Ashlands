/*
 * procgen.h — Procedural generation interface
 * C11, MIT License
 */
#ifndef ASHLANDS_PROCGEN_H
#define ASHLANDS_PROCGEN_H

#include "../../include/ashlands.h"
#include "../world.h"
#include "../ecs.h"

/* =========================================================
 * Dungeon generator (BSP)
 * ========================================================= */
typedef struct {
    int map_w, map_h;
    int min_room_w, min_room_h;
    int max_room_w, max_room_h;
    int max_depth;          /* BSP split depth */
    uint32_t seed;
} DungeonParams;

void dungeon_generate(GameMap *map, WorldState *ws,
                      const DungeonParams *params);

/* =========================================================
 * Overworld generator (placeholder for Phase 2)
 * ========================================================= */
void overworld_generate(GameMap *map, WorldState *ws, uint32_t seed);

/* =========================================================
 * LCG random number generator (portable, reproducible)
 * ========================================================= */
typedef struct { uint32_t state; } RNG;

void     rng_seed(RNG *r, uint32_t seed);
uint32_t rng_next(RNG *r);
int      rng_range(RNG *r, int lo, int hi);  /* [lo, hi] inclusive */
float    rng_float(RNG *r);                  /* [0, 1) */

#endif /* ASHLANDS_PROCGEN_H */
