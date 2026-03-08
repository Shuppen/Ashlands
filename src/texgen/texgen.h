#ifndef ASHLANDS_TEXGEN_H
#define ASHLANDS_TEXGEN_H

#include <stdbool.h>
#include <stddef.h>

#include "../../include/ashlands.h"

struct lua_State;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Surface SDL_Surface;
typedef struct SDL_Texture SDL_Texture;

#define TEXGEN_MAX_LAYERS 8
#define TEXGEN_MAX_TEXTURES 128
#define TEXGEN_NAME_LEN 64

typedef enum {
    TEX_LAYER_PERLIN = 0,
    TEX_LAYER_SIMPLEX,
    TEX_LAYER_VORONOI,
    TEX_LAYER_CELLULAR,
    TEX_LAYER_GRADIENT_LINEAR,
    TEX_LAYER_GRADIENT_RADIAL,
    TEX_LAYER_GRADIENT_ANGULAR,
} TexLayerType;

typedef enum {
    TEX_BLEND_NORMAL = 0,
    TEX_BLEND_MULTIPLY,
    TEX_BLEND_OVERLAY,
    TEX_BLEND_ADD,
    TEX_BLEND_SUBTRACT,
} TexBlendMode;

typedef struct {
    TexLayerType type;
    TexBlendMode blend;
    float scale;
    int octaves;
    float persistence;
    float lacunarity;
    float intensity;
    float edge_width;
    float angle;
    int iterations;
    bool has_color;
    bool has_color_var;
    bool has_edge_color;
    float color[3];
    float color_var[3];
    float edge_color[3];
} TexLayer;

typedef struct {
    char name[TEXGEN_NAME_LEN];
    int size;
    float base_color[3];
    TexLayer layers[TEXGEN_MAX_LAYERS];
    int layer_count;
    bool tiling;
    bool normal_map;
    float roughness;
} TexParams;

typedef struct {
    unsigned char *pixels;
    int width;
    int height;
    int channels;
} TexImage;

void texgen_init(void);
void texgen_shutdown(void);

TexParams texgen_parse_lua(struct lua_State *L, int index);
int texgen_register_lua(struct lua_State *L);

bool texgen_register(const char *name, const TexParams *params);
const TexParams *texgen_get(const char *name);
int texgen_count(void);
int texgen_lookup_tile_texture(TileType type, char *out_name, size_t out_size);

TexImage texgen_generate(const TexParams *params, int seed);
void teximage_free(TexImage *image);

SDL_Surface *texgen_generate_surface(const TexParams *params, int seed);
SDL_Texture *texgen_create_sdl(SDL_Renderer *ren, TexParams *params, int seed);
unsigned int texgen_create_gl(TexParams *params, int seed);
SDL_Surface *texgen_normal_map(SDL_Surface *heightmap);

#endif
