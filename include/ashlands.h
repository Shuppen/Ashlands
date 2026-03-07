/*
 * ashlands.h — Global types, constants, and macros for Ashlands engine
 * C11, MIT License
 */
#ifndef ASHLANDS_H
#define ASHLANDS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* =========================================================
 * Version
 * ========================================================= */
#define ASHLANDS_VERSION_MAJOR 0
#define ASHLANDS_VERSION_MINOR 1
#define ASHLANDS_VERSION_PATCH 0
#define ASHLANDS_VERSION_STR   "0.1.0"

/* =========================================================
 * Engine limits
 * ========================================================= */
#define MAX_ENTITIES        4096
#define MAX_ENTITY_TAGS     8
#define TAG_NAME_LEN        32

#define CHUNK_W             64
#define CHUNK_H             64
#define WORLD_CHUNKS_W      64   /* 64*64 chunks = 4096 tiles wide */
#define WORLD_CHUNKS_H      64

#define MAX_CHUNKS_LOADED   64   /* LRU cache */
#define SCRIPT_PATH_LEN     128
#define ITEM_NAME_LEN       64

#define MAX_INVENTORY_SLOTS 40
#define MAX_RECIPE_INGREDIENTS 8

/* =========================================================
 * Tile types
 * ========================================================= */
typedef enum {
    TILE_NONE       = 0,
    TILE_FLOOR      = 1,
    TILE_WALL       = 2,
    TILE_WATER      = 3,
    TILE_LAVA       = 4,
    TILE_DOOR_OPEN  = 5,
    TILE_DOOR_SHUT  = 6,
    TILE_STAIRS_UP  = 7,
    TILE_STAIRS_DOWN = 8,
    TILE_ASH        = 9,
    TILE_GRASS      = 10,
    TILE_SAND       = 11,
    TILE_TREE       = 12,
    TILE_RUBBLE     = 13,
    TILE_COUNT
} TileType;

/* =========================================================
 * ECS component bitmasks
 * ========================================================= */
#define COMP_POSITION    (1ULL << 0)
#define COMP_HEALTH      (1ULL << 1)
#define COMP_RENDER      (1ULL << 2)
#define COMP_STATS       (1ULL << 3)
#define COMP_SCRIPT      (1ULL << 4)
#define COMP_INVENTORY   (1ULL << 5)
#define COMP_FACTION     (1ULL << 6)
#define COMP_AI          (1ULL << 7)
#define COMP_PHYSICS     (1ULL << 8)
#define COMP_PLAYER      (1ULL << 9)
#define COMP_ITEM        (1ULL << 10)

/* =========================================================
 * ECS components — pure data, no logic
 * ========================================================= */
typedef struct {
    int x, y, z;
} PositionComponent;

typedef struct {
    int current, max;
} HealthComponent;

typedef struct {
    char glyph;
    uint32_t fg_color;   /* RGBA packed */
    uint32_t bg_color;
    int sprite_id;
} RenderComponent;

typedef struct {
    int attack, defense, speed;
    int level, xp;
} StatsComponent;

typedef struct {
    char script_path[SCRIPT_PATH_LEN];
    int  lua_ref;        /* LuaJIT registry ref, -1 if not loaded */
} ScriptComponent;

typedef struct {
    int slots[MAX_INVENTORY_SLOTS];  /* item entity IDs, -1 = empty */
    int count;
} InventoryComponent;

typedef struct {
    int faction_id;
    int hostility;       /* 0 = friendly, 100 = always hostile */
} FactionComponent;

typedef struct {
    int state;           /* AI state machine */
    int target_id;       /* entity ID of current target */
    int wander_x, wander_y;
    int aggression;      /* 0..100 */
} AIComponent;

typedef struct {
    int vx, vy;
    bool on_ground;
} PhysicsComponent;

/* Item-specific data */
typedef struct {
    char name[ITEM_NAME_LEN];
    int  type;
    int  value;
    int  weight;
    int  stack_size;
    int  stack_count;
} ItemComponent;

/* =========================================================
 * Entity
 * ========================================================= */
typedef struct {
    uint32_t id;
    uint64_t mask;       /* which components are present */
    bool     active;
    char     tags[MAX_ENTITY_TAGS][TAG_NAME_LEN];
    int      tag_count;
} Entity;

/* =========================================================
 * Tile (for a map cell)
 * ========================================================= */
typedef struct {
    TileType type;
    bool     explored;
    bool     visible;
    int      entity_id;  /* top entity on this tile, -1 if none */
    uint8_t  variant;    /* texture variant seed */
} Tile;

/* =========================================================
 * Chunk
 * ========================================================= */
typedef struct {
    int cx, cy;          /* chunk coordinates */
    bool generated;
    bool dirty;          /* needs re-render */
    Tile tiles[CHUNK_W][CHUNK_H];
} Chunk;

/* =========================================================
 * Utility macros
 * ========================================================= */
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))
#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(a) ((a) < 0 ? -(a) : (a))

/* Pack/unpack RGBA */
#define RGBA(r, g, b, a) \
    ((uint32_t)(r) | ((uint32_t)(g) << 8) | \
     ((uint32_t)(b) << 16) | ((uint32_t)(a) << 24))

#define RGBA_R(c) ((uint8_t)((c) & 0xFF))
#define RGBA_G(c) ((uint8_t)(((c) >> 8) & 0xFF))
#define RGBA_B(c) ((uint8_t)(((c) >> 16) & 0xFF))
#define RGBA_A(c) ((uint8_t)(((c) >> 24) & 0xFF))

/* Common colours */
#define COL_WHITE    RGBA(255, 255, 255, 255)
#define COL_BLACK    RGBA(0,   0,   0,   255)
#define COL_RED      RGBA(220, 50,  50,  255)
#define COL_GREEN    RGBA(50,  200, 50,  255)
#define COL_BLUE     RGBA(80,  120, 220, 255)
#define COL_YELLOW   RGBA(220, 200, 60,  255)
#define COL_GRAY     RGBA(160, 160, 160, 255)
#define COL_DARKGRAY RGBA(80,  80,  80,  255)
#define COL_BROWN    RGBA(160, 100, 60,  255)
#define COL_CYAN     RGBA(80,  210, 210, 255)

#endif /* ASHLANDS_H */
