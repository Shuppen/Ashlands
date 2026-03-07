/*
 * dungeon.c — BSP dungeon generator
 * C11, MIT License
 *
 * Binary Space Partitioning:
 * 1. Recursively split the map into two halves (horizontal or vertical)
 * 2. Place a room in each leaf node
 * 3. Connect sibling rooms with corridors (bottom-up)
 */
#include "procgen.h"

#include <stdlib.h>
#include <string.h>

/* =========================================================
 * LCG RNG
 * ========================================================= */
void rng_seed(RNG *r, uint32_t seed) {
    r->state = seed ? seed : 0xDEADBEEFu;
}

uint32_t rng_next(RNG *r) {
    /* Classic LCG constants (Knuth) */
    r->state = r->state * 1664525u + 1013904223u;
    return r->state;
}

int rng_range(RNG *r, int lo, int hi) {
    if (lo >= hi) return lo;
    return lo + (int)(rng_next(r) % (uint32_t)(hi - lo + 1));
}

float rng_float(RNG *r) {
    return (float)(rng_next(r) & 0xFFFFFF) / (float)0x1000000;
}

/* =========================================================
 * BSP node
 * ========================================================= */
#define MAX_BSP_NODES 512

typedef struct BSPNode {
    int x, y, w, h;         /* region */
    int room_x, room_y;     /* room top-left */
    int room_w, room_h;
    int left, right;        /* child indices, -1 = leaf */
    int center_x, center_y; /* for corridor connection */
} BSPNode;

typedef struct {
    BSPNode nodes[MAX_BSP_NODES];
    int     count;
} BSPTree;

static int bsp_new(BSPTree *t, int x, int y, int w, int h) {
    if (t->count >= MAX_BSP_NODES) return -1;
    int idx = t->count++;
    BSPNode *n = &t->nodes[idx];
    n->x       = x; n->y = y; n->w = w; n->h = h;
    n->left    = -1; n->right = -1;
    n->room_x  = 0; n->room_y  = 0;
    n->room_w  = 0; n->room_h  = 0;
    n->center_x = x + w / 2;
    n->center_y = y + h / 2;
    return idx;
}

/* Split a node. Returns false if too small to split. */
static bool bsp_split(BSPTree *t, int idx, int depth,
                       const DungeonParams *p, RNG *rng)
{
    BSPNode *n = &t->nodes[idx];
    if (depth <= 0) return false;

    bool split_h = rng_range(rng, 0, 1) == 0; /* horizontal split */

    /* Force orientation if node is too wide or tall */
    if (n->w > n->h * 1.25f) split_h = false;
    if (n->h > n->w * 1.25f) split_h = true;

    int min_size = split_h ? p->min_room_h * 2 + 2
                           : p->min_room_w * 2 + 2;
    int region   = split_h ? n->h : n->w;
    if (region < min_size) return false;

    int split_at;
    if (split_h) {
        split_at = rng_range(rng, p->min_room_h + 1, n->h - p->min_room_h - 1);
        n->left  = bsp_new(t, n->x, n->y,          n->w, split_at);
        n->right = bsp_new(t, n->x, n->y + split_at, n->w, n->h - split_at);
    } else {
        split_at = rng_range(rng, p->min_room_w + 1, n->w - p->min_room_w - 1);
        n->left  = bsp_new(t, n->x,           n->y, split_at,       n->h);
        n->right = bsp_new(t, n->x + split_at, n->y, n->w - split_at, n->h);
    }

    if (n->left < 0 || n->right < 0) return false;

    bsp_split(t, n->left,  depth - 1, p, rng);
    bsp_split(t, n->right, depth - 1, p, rng);
    return true;
}

/* =========================================================
 * Carve rooms and corridors
 * ========================================================= */

static void carve_room(GameMap *m, int rx, int ry, int rw, int rh) {
    for (int y = ry; y < ry + rh; y++) {
        for (int x = rx; x < rx + rw; x++) {
            if (map_in_bounds(m, x, y))
                m->tiles[x][y].type = TILE_FLOOR;
        }
    }
}

static void carve_h_corridor(GameMap *m, int x1, int x2, int y) {
    int start = x1 < x2 ? x1 : x2;
    int end   = x1 < x2 ? x2 : x1;
    for (int x = start; x <= end; x++) {
        if (map_in_bounds(m, x, y))
            m->tiles[x][y].type = TILE_FLOOR;
    }
}

static void carve_v_corridor(GameMap *m, int y1, int y2, int x) {
    int start = y1 < y2 ? y1 : y2;
    int end   = y1 < y2 ? y2 : y1;
    for (int y = start; y <= end; y++) {
        if (map_in_bounds(m, x, y))
            m->tiles[x][y].type = TILE_FLOOR;
    }
}

static void place_rooms(BSPTree *t, int idx,
                         const DungeonParams *p, RNG *rng,
                         GameMap *m)
{
    if (idx < 0) return;
    BSPNode *n = &t->nodes[idx];

    if (n->left < 0 && n->right < 0) {
        /* Leaf — place a room */
        int rw = rng_range(rng, p->min_room_w,
                           MIN(p->max_room_w, n->w - 2));
        int rh = rng_range(rng, p->min_room_h,
                           MIN(p->max_room_h, n->h - 2));
        int rx = n->x + rng_range(rng, 1, n->w - rw - 1);
        int ry = n->y + rng_range(rng, 1, n->h - rh - 1);

        rw = MAX(rw, p->min_room_w);
        rh = MAX(rh, p->min_room_h);

        n->room_x = rx; n->room_y = ry;
        n->room_w = rw; n->room_h = rh;
        n->center_x = rx + rw / 2;
        n->center_y = ry + rh / 2;

        carve_room(m, rx, ry, rw, rh);

        /* Place door variants (25% chance) */
        if (rng_range(rng, 0, 3) == 0) {
            int dx = rx + rng_range(rng, 0, rw - 1);
            int dy = (rng_range(rng, 0, 1) == 0) ? ry - 1 : ry + rh;
            if (map_in_bounds(m, dx, dy) &&
                m->tiles[dx][dy].type == TILE_WALL)
            {
                m->tiles[dx][dy].type = TILE_DOOR_SHUT;
            }
        }
    } else {
        place_rooms(t, n->left,  p, rng, m);
        place_rooms(t, n->right, p, rng, m);

        /* Connect left and right sub-trees */
        int lc = -1, rc = -1;
        /* Find a leaf in each sub-tree */
        int li = n->left;
        while (t->nodes[li].left >= 0) li = t->nodes[li].left;
        lc = li;

        int ri = n->right;
        while (t->nodes[ri].left >= 0) ri = t->nodes[ri].left;
        rc = ri;

        if (lc >= 0 && rc >= 0) {
            int x1 = t->nodes[lc].center_x;
            int y1 = t->nodes[lc].center_y;
            int x2 = t->nodes[rc].center_x;
            int y2 = t->nodes[rc].center_y;

            /* L-shaped corridor */
            if (rng_range(rng, 0, 1) == 0) {
                carve_h_corridor(m, x1, x2, y1);
                carve_v_corridor(m, y1, y2, x2);
            } else {
                carve_v_corridor(m, y1, y2, x1);
                carve_h_corridor(m, x1, x2, y2);
            }
        }

        /* Update this node's center */
        if (lc >= 0) {
            n->center_x = t->nodes[lc].center_x;
            n->center_y = t->nodes[lc].center_y;
        }
    }
}

/* =========================================================
 * Add walls around floor tiles
 * ========================================================= */
static void add_walls(GameMap *m) {
    /* Make a copy of floor tiles first */
    static TileType tmp[MAP_MAX_W][MAP_MAX_H];
    for (int x = 0; x < m->width; x++)
        for (int y = 0; y < m->height; y++)
            tmp[x][y] = m->tiles[x][y].type;

    static const int dirs[8][2] = {
        {-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{0,1},{1,1}
    };

    for (int x = 0; x < m->width; x++) {
        for (int y = 0; y < m->height; y++) {
            if (tmp[x][y] != TILE_NONE) continue;
            /* Check 8 neighbours */
            for (int d = 0; d < 8; d++) {
                int nx = x + dirs[d][0];
                int ny = y + dirs[d][1];
                if (map_in_bounds(m, nx, ny) &&
                    tmp[nx][ny] == TILE_FLOOR)
                {
                    m->tiles[x][y].type = TILE_WALL;
                    break;
                }
            }
        }
    }
}

/* =========================================================
 * Place stairs
 * ========================================================= */
static void place_stairs(BSPTree *t, GameMap *m, RNG *rng) {
    /* Find leftmost and rightmost leaf rooms */
    int first = 0;
    while (t->nodes[first].left >= 0) first = t->nodes[first].left;

    int last = 0;
    while (t->nodes[last].right >= 0) last = t->nodes[last].right;

    BSPNode *fn = &t->nodes[first];
    BSPNode *ln = &t->nodes[last];

    if (fn->room_w > 0 && fn->room_h > 0) {
        int sx = fn->room_x + rng_range(rng, 0, fn->room_w - 1);
        int sy = fn->room_y + rng_range(rng, 0, fn->room_h - 1);
        if (map_in_bounds(m, sx, sy))
            m->tiles[sx][sy].type = TILE_STAIRS_DOWN;
        m->spawn_x = fn->center_x;
        m->spawn_y = fn->center_y;
    }

    if (ln->room_w > 0 && ln->room_h > 0) {
        int sx = ln->room_x + rng_range(rng, 0, ln->room_w - 1);
        int sy = ln->room_y + rng_range(rng, 0, ln->room_h - 1);
        if (map_in_bounds(m, sx, sy))
            m->tiles[sx][sy].type = TILE_STAIRS_UP;
    }
}

/* =========================================================
 * Public entry point
 * ========================================================= */
void dungeon_generate(GameMap *map, WorldState *ws,
                      const DungeonParams *params)
{
    RNG rng;
    rng_seed(&rng, params->seed ^ ws->seed);

    map_clear(map, params->map_w, params->map_h, TILE_NONE);

    BSPTree tree;
    memset(&tree, 0, sizeof(BSPTree));
    int root = bsp_new(&tree, 1, 1, params->map_w - 2, params->map_h - 2);
    bsp_split(&tree, root, params->max_depth, params, &rng);

    place_rooms(&tree, root, params, &rng, map);
    add_walls(map);
    place_stairs(&tree, map, &rng);

    map->generated = true;
}

/* =========================================================
 * Overworld stub
 * ========================================================= */
void overworld_generate(GameMap *map, WorldState *ws, uint32_t seed) {
    (void)ws;
    map_clear(map, 128, 128, TILE_ASH);
    /* Simple: fill interior with grass patches */
    RNG rng;
    rng_seed(&rng, seed);
    for (int i = 0; i < 200; i++) {
        int x = rng_range(&rng, 5, 120);
        int y = rng_range(&rng, 5, 120);
        int r = rng_range(&rng, 2, 8);
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (dx*dx + dy*dy <= r*r && map_in_bounds(map, x+dx, y+dy))
                    map->tiles[x+dx][y+dy].type = TILE_GRASS;
            }
        }
    }
    map->spawn_x = 64;
    map->spawn_y = 64;
    map->generated = true;
}
