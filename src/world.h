/*
 * world.h — Map, chunks, and world-state interface
 * C11, MIT License
 */
#ifndef ASHLANDS_WORLD_H
#define ASHLANDS_WORLD_H

#include "../include/ashlands.h"
#include "ecs.h"

/* =========================================================
 * Time and weather
 * ========================================================= */
typedef enum {
    WEATHER_CLEAR  = 0,
    WEATHER_ASH,
    WEATHER_RAIN,
    WEATHER_STORM,
    WEATHER_COUNT
} WeatherType;

typedef struct {
    int hour;       /* 0..23 */
    int day;        /* 0-based */
    int season;     /* 0=spring,1=summer,2=autumn,3=winter */
    int tick;       /* ticks within current hour */
    int ticks_per_hour;
} WorldTime;

/* =========================================================
 * Chunk cache (LRU)
 * ========================================================= */
typedef struct {
    Chunk   chunks[MAX_CHUNKS_LOADED];
    int     lru_order[MAX_CHUNKS_LOADED]; /* indices into chunks[] */
    int     count;
} ChunkCache;

/* =========================================================
 * GameMap — a dungeon floor or overworld region
 * ========================================================= */
#define MAP_MAX_W 256
#define MAP_MAX_H 256

typedef struct {
    int    width, height;
    Tile   tiles[MAP_MAX_W][MAP_MAX_H];
    int    spawn_x, spawn_y;     /* default player start */
    bool   generated;
} GameMap;

/* =========================================================
 * WorldState
 * ========================================================= */
typedef struct {
    World      *ecs;
    GameMap     map;
    WorldTime   time;
    WeatherType weather;
    uint32_t    seed;
} WorldState;

/* =========================================================
 * World lifecycle
 * ========================================================= */
WorldState *world_state_create(uint32_t seed);
void        world_state_destroy(WorldState *ws);

/* =========================================================
 * Map helpers
 * ========================================================= */
bool   map_in_bounds(const GameMap *m, int x, int y);
Tile  *map_tile(GameMap *m, int x, int y);   /* NULL if out of bounds */
bool   map_is_walkable(const GameMap *m, int x, int y);
bool   map_is_transparent(const GameMap *m, int x, int y);

void   map_clear(GameMap *m, int w, int h, TileType fill);

/* Basic flood-fill field-of-view (shadowcast) */
void   map_compute_fov(GameMap *m, int ox, int oy, int radius);

/* =========================================================
 * Time helpers
 * ========================================================= */
void   world_time_tick(WorldState *ws);
bool   world_is_night(const WorldState *ws);

#endif /* ASHLANDS_WORLD_H */
