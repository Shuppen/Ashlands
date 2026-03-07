/*
 * world.c — Map, chunks, and world-state implementation
 * C11, MIT License
 */
#include "world.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* =========================================================
 * World lifecycle
 * ========================================================= */

WorldState *world_state_create(uint32_t seed) {
    WorldState *ws = calloc(1, sizeof(WorldState));
    if (!ws) return NULL;

    ws->ecs  = world_create();
    if (!ws->ecs) { free(ws); return NULL; }

    ws->seed    = seed;
    ws->weather = WEATHER_CLEAR;

    ws->time.hour           = 6;
    ws->time.day            = 0;
    ws->time.season         = 0;
    ws->time.tick           = 0;
    ws->time.ticks_per_hour = 100;

    return ws;
}

void world_state_destroy(WorldState *ws) {
    if (!ws) return;
    world_destroy(ws->ecs);
    free(ws);
}

/* =========================================================
 * Map helpers
 * ========================================================= */

bool map_in_bounds(const GameMap *m, int x, int y) {
    return x >= 0 && y >= 0 && x < m->width && y < m->height;
}

Tile *map_tile(GameMap *m, int x, int y) {
    if (!map_in_bounds(m, x, y)) return NULL;
    return &m->tiles[x][y];
}

bool map_is_walkable(const GameMap *m, int x, int y) {
    if (!map_in_bounds(m, x, y)) return false;
    TileType t = m->tiles[x][y].type;
    return t == TILE_FLOOR   ||
           t == TILE_ASH     ||
           t == TILE_GRASS   ||
           t == TILE_SAND    ||
           t == TILE_DOOR_OPEN ||
           t == TILE_STAIRS_UP ||
           t == TILE_STAIRS_DOWN;
}

bool map_is_transparent(const GameMap *m, int x, int y) {
    if (!map_in_bounds(m, x, y)) return false;
    TileType t = m->tiles[x][y].type;
    return t != TILE_WALL && t != TILE_TREE && t != TILE_DOOR_SHUT;
}

void map_clear(GameMap *m, int w, int h, TileType fill) {
    m->width     = w;
    m->height    = h;
    m->generated = false;
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            m->tiles[x][y].type     = fill;
            m->tiles[x][y].explored = false;
            m->tiles[x][y].visible  = false;
            m->tiles[x][y].entity_id = -1;
            m->tiles[x][y].variant  = (uint8_t)((x * 31 + y * 17) & 0xFF);
        }
    }
}

/* =========================================================
 * Field of view — simple recursive shadowcasting (octant 0..7)
 * ========================================================= */

/* Multipliers for octant transformation */
static const int row_mult[8]  = { 1,  0,  0, -1, -1,  0,  0,  1 };
static const int col_mult[8]  = { 0,  1, -1,  0,  0, -1,  1,  0 };
static const int row_mult2[8] = { 0,  1,  1,  0,  0, -1, -1,  0 };
static const int col_mult2[8] = { 1,  0,  0,  1, -1,  0,  0, -1 };

static void fov_cast_light(GameMap *m, int cx, int cy,
                            int row, float start_slope, float end_slope,
                            int radius,
                            int xx, int xy, int yx, int yy)
{
    if (start_slope < end_slope) return;

    float next_start = start_slope;
    bool  blocked    = false;

    for (int dist = row; dist <= radius && !blocked; dist++) {
        for (int dx = -dist; dx <= 0; dx++) {
            int dy = -dist;
            int mx = cx + dx * xx + dy * xy;
            int my = cy + dx * yx + dy * yy;

            float l_slope = ((float)dx - 0.5f) / ((float)dy + 0.5f);
            float r_slope = ((float)dx + 0.5f) / ((float)dy - 0.5f);

            if (start_slope < r_slope) continue;
            if (end_slope > l_slope)   break;

            float dist_sq = (float)(dx*dx + dy*dy);
            if (dist_sq <= (float)(radius * radius)) {
                if (map_in_bounds(m, mx, my)) {
                    m->tiles[mx][my].visible  = true;
                    m->tiles[mx][my].explored = true;
                }
            }

            if (blocked) {
                if (!map_is_transparent(m, mx, my)) {
                    next_start = r_slope;
                } else {
                    blocked = false;
                    start_slope = next_start;
                }
            } else if (!map_is_transparent(m, mx, my) && dist < radius) {
                blocked = true;
                fov_cast_light(m, cx, cy, dist + 1, start_slope, l_slope,
                               radius, xx, xy, yx, yy);
                next_start = r_slope;
            }
        }
    }
    (void)row_mult; (void)col_mult; (void)row_mult2; (void)col_mult2;
}

void map_compute_fov(GameMap *m, int ox, int oy, int radius) {
    /* Reset visibility */
    for (int x = 0; x < m->width; x++)
        for (int y = 0; y < m->height; y++)
            m->tiles[x][y].visible = false;

    if (map_in_bounds(m, ox, oy)) {
        m->tiles[ox][oy].visible  = true;
        m->tiles[ox][oy].explored = true;
    }

    /* Cast light for all 8 octants */
    static const int mult[4][8] = {
        { 1,  0,  0, -1, -1,  0,  0,  1 },
        { 0,  1, -1,  0,  0, -1,  1,  0 },
        { 0,  1,  1,  0,  0, -1, -1,  0 },
        { 1,  0,  0,  1, -1,  0,  0, -1 }
    };
    for (int oct = 0; oct < 8; oct++) {
        fov_cast_light(m, ox, oy, 1, 1.0f, 0.0f, radius,
                       mult[0][oct], mult[1][oct],
                       mult[2][oct], mult[3][oct]);
    }
}

/* =========================================================
 * Time helpers
 * ========================================================= */

void world_time_tick(WorldState *ws) {
    WorldTime *t = &ws->time;
    t->tick++;
    if (t->tick >= t->ticks_per_hour) {
        t->tick = 0;
        t->hour++;
        if (t->hour >= 24) {
            t->hour = 0;
            t->day++;
            if (t->day >= 90) {   /* 90 days per season */
                t->day    = 0;
                t->season = (t->season + 1) % 4;
            }
        }
    }
}

bool world_is_night(const WorldState *ws) {
    int h = ws->time.hour;
    return h >= 20 || h < 6;
}
