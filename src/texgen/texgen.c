#include "texgen.h"

#include "noise.h"
#include "texcache.h"

#include "../lua_compat.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#ifdef PLATFORM_WEB
#include <SDL2/SDL_opengles2.h>
#else
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#include <SDL2/SDL_opengl.h>
#endif

static float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

typedef struct {
    TexParams defs[TEXGEN_MAX_TEXTURES];
    int count;
} TexRegistry;

static TexRegistry s_registry;

static float clamp01(float v) {
    return CLAMP(v, 0.0f, 1.0f);
}

static void default_texture_presets(void) {
    TexParams ash_stone;
    TexParams ash_floor;
    TexParams dead_grass;
    TexParams ruin_wall;

    if (texgen_get("ash_stone")) {
        return;
    }

    memset(&ash_stone, 0, sizeof(ash_stone));
    strncpy(ash_stone.name, "ash_stone", TEXGEN_NAME_LEN - 1);
    ash_stone.size = 32;
    ash_stone.tiling = true;
    ash_stone.base_color[0] = 0.35f;
    ash_stone.base_color[1] = 0.32f;
    ash_stone.base_color[2] = 0.28f;
    ash_stone.layer_count = 2;
    ash_stone.layers[0].type = TEX_LAYER_PERLIN;
    ash_stone.layers[0].scale = 4.0f;
    ash_stone.layers[0].intensity = 0.30f;
    ash_stone.layers[0].has_color_var = true;
    ash_stone.layers[0].color_var[0] = 0.05f;
    ash_stone.layers[0].color_var[1] = 0.05f;
    ash_stone.layers[0].color_var[2] = 0.03f;
    ash_stone.layers[1].type = TEX_LAYER_VORONOI;
    ash_stone.layers[1].scale = 6.0f;
    ash_stone.layers[1].edge_width = 0.05f;
    ash_stone.layers[1].intensity = 0.45f;
    ash_stone.layers[1].has_edge_color = true;
    ash_stone.layers[1].edge_color[0] = 0.15f;
    ash_stone.layers[1].edge_color[1] = 0.12f;
    ash_stone.layers[1].edge_color[2] = 0.10f;
    texgen_register(ash_stone.name, &ash_stone);

    memset(&ash_floor, 0, sizeof(ash_floor));
    strncpy(ash_floor.name, "ash_floor", TEXGEN_NAME_LEN - 1);
    ash_floor.size = 32;
    ash_floor.tiling = true;
    ash_floor.base_color[0] = 0.50f;
    ash_floor.base_color[1] = 0.48f;
    ash_floor.base_color[2] = 0.44f;
    ash_floor.layer_count = 2;
    ash_floor.layers[0].type = TEX_LAYER_PERLIN;
    ash_floor.layers[0].scale = 3.0f;
    ash_floor.layers[0].intensity = 0.20f;
    ash_floor.layers[0].has_color_var = true;
    ash_floor.layers[0].color_var[0] = 0.04f;
    ash_floor.layers[0].color_var[1] = 0.04f;
    ash_floor.layers[0].color_var[2] = 0.03f;
    ash_floor.layers[1].type = TEX_LAYER_PERLIN;
    ash_floor.layers[1].scale = 8.0f;
    ash_floor.layers[1].intensity = 0.12f;
    ash_floor.layers[1].has_color = true;
    ash_floor.layers[1].color[0] = 0.40f;
    ash_floor.layers[1].color[1] = 0.38f;
    ash_floor.layers[1].color[2] = 0.35f;
    texgen_register(ash_floor.name, &ash_floor);

    memset(&dead_grass, 0, sizeof(dead_grass));
    strncpy(dead_grass.name, "dead_grass", TEXGEN_NAME_LEN - 1);
    dead_grass.size = 32;
    dead_grass.tiling = true;
    dead_grass.base_color[0] = 0.28f;
    dead_grass.base_color[1] = 0.26f;
    dead_grass.base_color[2] = 0.18f;
    dead_grass.layer_count = 2;
    dead_grass.layers[0].type = TEX_LAYER_PERLIN;
    dead_grass.layers[0].scale = 5.0f;
    dead_grass.layers[0].intensity = 0.25f;
    dead_grass.layers[0].has_color_var = true;
    dead_grass.layers[0].color_var[0] = 0.06f;
    dead_grass.layers[0].color_var[1] = 0.05f;
    dead_grass.layers[0].color_var[2] = 0.03f;
    dead_grass.layers[1].type = TEX_LAYER_CELLULAR;
    dead_grass.layers[1].scale = 10.0f;
    dead_grass.layers[1].intensity = 0.15f;
    dead_grass.layers[1].iterations = 4;
    dead_grass.layers[1].has_color = true;
    dead_grass.layers[1].color[0] = 0.22f;
    dead_grass.layers[1].color[1] = 0.20f;
    dead_grass.layers[1].color[2] = 0.12f;
    texgen_register(dead_grass.name, &dead_grass);

    memset(&ruin_wall, 0, sizeof(ruin_wall));
    strncpy(ruin_wall.name, "ruin_wall", TEXGEN_NAME_LEN - 1);
    ruin_wall.size = 32;
    ruin_wall.tiling = true;
    ruin_wall.normal_map = true;
    ruin_wall.base_color[0] = 0.45f;
    ruin_wall.base_color[1] = 0.38f;
    ruin_wall.base_color[2] = 0.30f;
    ruin_wall.layer_count = 2;
    ruin_wall.layers[0].type = TEX_LAYER_VORONOI;
    ruin_wall.layers[0].scale = 4.0f;
    ruin_wall.layers[0].edge_width = 0.08f;
    ruin_wall.layers[0].intensity = 0.45f;
    ruin_wall.layers[0].has_edge_color = true;
    ruin_wall.layers[0].edge_color[0] = 0.20f;
    ruin_wall.layers[0].edge_color[1] = 0.15f;
    ruin_wall.layers[0].edge_color[2] = 0.10f;
    ruin_wall.layers[1].type = TEX_LAYER_PERLIN;
    ruin_wall.layers[1].scale = 8.0f;
    ruin_wall.layers[1].intensity = 0.10f;
    ruin_wall.layers[1].has_color_var = true;
    ruin_wall.layers[1].color_var[0] = 0.05f;
    ruin_wall.layers[1].color_var[1] = 0.04f;
    ruin_wall.layers[1].color_var[2] = 0.03f;
    texgen_register(ruin_wall.name, &ruin_wall);
}

static float blend_value(float base, float layer, TexBlendMode mode) {
    switch (mode) {
    case TEX_BLEND_MULTIPLY:
        return base * layer;
    case TEX_BLEND_OVERLAY:
        return base < 0.5f ? 2.0f * base * layer
                           : 1.0f - 2.0f * (1.0f - base) * (1.0f - layer);
    case TEX_BLEND_ADD:
        return clamp01(base + layer);
    case TEX_BLEND_SUBTRACT:
        return clamp01(base - layer);
    case TEX_BLEND_NORMAL:
    default:
        return layer;
    }
}

static float lua_table_number(struct lua_State *L, int index,
                              const char *key, float fallback) {
    float value = fallback;

    lua_getfield(L, index, key);
    if (lua_isnumber(L, -1)) {
        value = (float)lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
    return value;
}

static bool lua_table_bool(struct lua_State *L, int index,
                           const char *key, bool fallback) {
    bool value = fallback;

    lua_getfield(L, index, key);
    if (lua_isboolean(L, -1)) {
        value = lua_toboolean(L, -1) != 0;
    }
    lua_pop(L, 1);
    return value;
}

static void lua_table_color3(struct lua_State *L, int index, const char *key,
                             float out[3], bool *present) {
    bool found = false;

    lua_getfield(L, index, key);
    if (lua_istable(L, -1)) {
        for (int i = 0; i < 3; i++) {
            lua_rawgeti(L, -1, i + 1);
            out[i] = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
            lua_pop(L, 1);
        }
        found = true;
    }
    lua_pop(L, 1);
    if (present) {
        *present = found;
    }
}

static TexBlendMode parse_blend_mode(const char *name) {
    if (!name || strcmp(name, "normal") == 0 || strcmp(name, "mix") == 0) {
        return TEX_BLEND_NORMAL;
    }
    if (strcmp(name, "multiply") == 0) {
        return TEX_BLEND_MULTIPLY;
    }
    if (strcmp(name, "overlay") == 0) {
        return TEX_BLEND_OVERLAY;
    }
    if (strcmp(name, "add") == 0) {
        return TEX_BLEND_ADD;
    }
    if (strcmp(name, "subtract") == 0) {
        return TEX_BLEND_SUBTRACT;
    }
    return TEX_BLEND_NORMAL;
}

static TexLayerType parse_layer_type(const char *name) {
    if (!name || strcmp(name, "perlin") == 0) {
        return TEX_LAYER_PERLIN;
    }
    if (strcmp(name, "simplex") == 0) {
        return TEX_LAYER_SIMPLEX;
    }
    if (strcmp(name, "voronoi") == 0) {
        return TEX_LAYER_VORONOI;
    }
    if (strcmp(name, "cellular") == 0) {
        return TEX_LAYER_CELLULAR;
    }
    if (strcmp(name, "gradient_linear") == 0 || strcmp(name, "linear") == 0) {
        return TEX_LAYER_GRADIENT_LINEAR;
    }
    if (strcmp(name, "gradient_radial") == 0 || strcmp(name, "radial") == 0) {
        return TEX_LAYER_GRADIENT_RADIAL;
    }
    if (strcmp(name, "gradient_angular") == 0 || strcmp(name, "angular") == 0) {
        return TEX_LAYER_GRADIENT_ANGULAR;
    }
    return TEX_LAYER_PERLIN;
}

static float sample_gradient(const TexLayer *layer, float nx, float ny) {
    switch (layer->type) {
    case TEX_LAYER_GRADIENT_RADIAL: {
        float dx = nx - 0.5f;
        float dy = ny - 0.5f;
        return clamp01(1.0f - sqrtf(dx * dx + dy * dy) * 1.4142f);
    }
    case TEX_LAYER_GRADIENT_ANGULAR: {
        float angle = atan2f(ny - 0.5f, nx - 0.5f) + 3.1415926f + layer->angle;
        return angle / 6.2831853f;
    }
    case TEX_LAYER_GRADIENT_LINEAR:
    default:
        return clamp01(nx * cosf(layer->angle) + ny * sinf(layer->angle));
    }
}

static float sample_layer_value(const TexLayer *layer,
                                float x, float y,
                                float nx, float ny,
                                int seed,
                                const float *cellular_map,
                                int size) {
    float edge_dist = 0.0f;

    switch (layer->type) {
    case TEX_LAYER_PERLIN:
        return 0.5f + 0.5f * noise_fbm(perlin2d, x, y, seed,
                                       layer->octaves,
                                       layer->persistence,
                                       layer->lacunarity);
    case TEX_LAYER_SIMPLEX:
        return 0.5f + 0.5f * noise_fbm(simplex2d, x, y, seed,
                                       layer->octaves,
                                       layer->persistence,
                                       layer->lacunarity);
    case TEX_LAYER_VORONOI: {
        float dist = voronoi2d(x, y, seed, &edge_dist);
        if (layer->edge_width > 0.0f) {
            return 1.0f - clamp01(edge_dist / layer->edge_width);
        }
        return clamp01(1.0f - dist);
    }
    case TEX_LAYER_CELLULAR:
        if (cellular_map && size > 0) {
            int ix = CLAMP((int)(nx * (float)(size - 1)), 0, size - 1);
            int iy = CLAMP((int)(ny * (float)(size - 1)), 0, size - 1);
            return cellular_map[iy * size + ix];
        }
        return 0.0f;
    case TEX_LAYER_GRADIENT_LINEAR:
    case TEX_LAYER_GRADIENT_RADIAL:
    case TEX_LAYER_GRADIENT_ANGULAR:
        return sample_gradient(layer, nx, ny);
    default:
        return 0.0f;
    }
}

void texgen_init(void) {
    memset(&s_registry, 0, sizeof(s_registry));
    texcache_init();
    default_texture_presets();
}

void texgen_shutdown(void) {
    memset(&s_registry, 0, sizeof(s_registry));
    texcache_clear();
}

bool texgen_register(const char *name, const TexParams *params) {
    int slot = -1;

    if (!name || !params || !name[0]) {
        return false;
    }

    for (int i = 0; i < s_registry.count; i++) {
        if (strcmp(s_registry.defs[i].name, name) == 0) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        if (s_registry.count >= TEXGEN_MAX_TEXTURES) {
            return false;
        }
        slot = s_registry.count++;
    }

    s_registry.defs[slot] = *params;
    strncpy(s_registry.defs[slot].name, name, TEXGEN_NAME_LEN - 1);
    s_registry.defs[slot].name[TEXGEN_NAME_LEN - 1] = '\0';
    return true;
}

const TexParams *texgen_get(const char *name) {
    for (int i = 0; i < s_registry.count; i++) {
        if (strcmp(s_registry.defs[i].name, name) == 0) {
            return &s_registry.defs[i];
        }
    }
    return NULL;
}

int texgen_count(void) {
    return s_registry.count;
}

int texgen_lookup_tile_texture(TileType type, char *out_name, size_t out_size) {
    const char *name = NULL;

    switch (type) {
    case TILE_WALL:
        name = "ash_stone";
        break;
    case TILE_ASH:
    case TILE_FLOOR:
        name = "ash_floor";
        break;
    case TILE_GRASS:
        name = "dead_grass";
        break;
    case TILE_TREE:
    case TILE_RUBBLE:
        name = "ruin_wall";
        break;
    default:
        name = NULL;
        break;
    }

    if (!out_name || out_size == 0 || !name) {
        return 0;
    }

    strncpy(out_name, name, out_size - 1);
    out_name[out_size - 1] = '\0';
    return 1;
}

TexParams texgen_parse_lua(struct lua_State *L, int index) {
    TexParams params;
    int abs_index = lua_absindex(L, index);

    memset(&params, 0, sizeof(params));
    params.size = (int)lua_table_number(L, abs_index, "size", 32.0f);
    params.size = CLAMP(params.size, 4, 128);
    params.tiling = lua_table_bool(L, abs_index, "tiling", true);
    params.normal_map = lua_table_bool(L, abs_index, "normal_map", false);
    params.roughness = lua_table_number(L, abs_index, "roughness", 0.7f);
    params.base_color[0] = 0.5f;
    params.base_color[1] = 0.5f;
    params.base_color[2] = 0.5f;
    lua_table_color3(L, abs_index, "base_color", params.base_color, NULL);

    lua_getfield(L, abs_index, "layers");
    if (lua_istable(L, -1)) {
        int count = (int)lua_rawlen(L, -1);
        params.layer_count = MIN(count, TEXGEN_MAX_LAYERS);

        for (int i = 0; i < params.layer_count; i++) {
            TexLayer *layer = &params.layers[i];
            const char *type_name;
            const char *blend_name;
            int layer_index;

            lua_rawgeti(L, -1, i + 1);
            layer_index = lua_gettop(L);
    lua_getfield(L, layer_index, "type");
            type_name = lua_tostring(L, -1);
            lua_pop(L, 1);
            lua_getfield(L, layer_index, "blend");
            blend_name = lua_tostring(L, -1);
            lua_pop(L, 1);
            layer->type = parse_layer_type(type_name);
            layer->blend = parse_blend_mode(blend_name);
            layer->scale = lua_table_number(L, layer_index, "scale", 4.0f);
            layer->octaves = (int)lua_table_number(L, layer_index, "octaves", 3.0f);
            layer->persistence = lua_table_number(L, layer_index, "persistence", 0.5f);
            layer->lacunarity = lua_table_number(L, layer_index, "lacunarity", 2.0f);
            layer->intensity = lua_table_number(L, layer_index, "intensity", 0.25f);
            layer->edge_width = lua_table_number(L, layer_index, "edge_width", 0.05f);
            layer->angle = lua_table_number(L, layer_index, "angle", 0.0f);
            layer->iterations = (int)lua_table_number(L, layer_index, "iterations", 4.0f);
            if (layer->octaves <= 0) {
                layer->octaves = 1;
            }
            if (layer->persistence <= 0.0f) {
                layer->persistence = 0.5f;
            }
            if (layer->lacunarity <= 0.0f) {
                layer->lacunarity = 2.0f;
            }
            if (layer->intensity <= 0.0f) {
                layer->intensity = 0.25f;
            }
            lua_table_color3(L, layer_index, "color", layer->color, &layer->has_color);
            lua_table_color3(L, layer_index, "color_var", layer->color_var,
                             &layer->has_color_var);
            lua_table_color3(L, layer_index, "edge_color", layer->edge_color,
                             &layer->has_edge_color);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    return params;
}

TexImage texgen_generate(const TexParams *params, int seed) {
    TexImage image;
    float *cellular = NULL;

    memset(&image, 0, sizeof(image));
    if (!params || params->size <= 0) {
        return image;
    }

    image.width = params->size;
    image.height = params->size;
    image.channels = 4;
    image.pixels = calloc((size_t)image.width * (size_t)image.height * 4u, 1);
    if (!image.pixels) {
        return image;
    }

    for (int li = 0; li < params->layer_count; li++) {
        const TexLayer *layer = &params->layers[li];
        if (layer->type == TEX_LAYER_CELLULAR) {
            if (!cellular) {
                cellular = malloc((size_t)image.width * (size_t)image.height * sizeof(*cellular));
            }
            if (cellular) {
                cellular_automata_generate(cellular, image.width, image.height,
                                           seed + li * 131, MAX(layer->iterations, 1));
            }
        }
    }

    for (int y = 0; y < image.height; y++) {
        for (int x = 0; x < image.width; x++) {
            float nx = (float)x / (float)MAX(image.width - 1, 1);
            float ny = (float)y / (float)MAX(image.height - 1, 1);
            float col[3] = {
                params->base_color[0],
                params->base_color[1],
                params->base_color[2]
            };

            for (int li = 0; li < params->layer_count; li++) {
                const TexLayer *layer = &params->layers[li];
                float scale = layer->scale > 0.001f ? layer->scale : 1.0f;
                float value = sample_layer_value(layer,
                                                 nx * scale,
                                                 ny * scale,
                                                 nx, ny,
                                                 seed + li * 257,
                                                 cellular,
                                                 image.width);
                float layer_col[3] = {
                    col[0], col[1], col[2]
                };

                if (layer->has_color) {
                    layer_col[0] = layer->color[0];
                    layer_col[1] = layer->color[1];
                    layer_col[2] = layer->color[2];
                } else if (layer->has_color_var) {
                    layer_col[0] = clamp01(col[0] + (value - 0.5f) * 2.0f * layer->color_var[0]);
                    layer_col[1] = clamp01(col[1] + (value - 0.5f) * 2.0f * layer->color_var[1]);
                    layer_col[2] = clamp01(col[2] + (value - 0.5f) * 2.0f * layer->color_var[2]);
                } else if (layer->has_edge_color && layer->type == TEX_LAYER_VORONOI) {
                    layer_col[0] = layer->edge_color[0];
                    layer_col[1] = layer->edge_color[1];
                    layer_col[2] = layer->edge_color[2];
                } else {
                    layer_col[0] = clamp01(col[0] + (value - 0.5f) * layer->intensity);
                    layer_col[1] = clamp01(col[1] + (value - 0.5f) * layer->intensity);
                    layer_col[2] = clamp01(col[2] + (value - 0.5f) * layer->intensity);
                }

                for (int c = 0; c < 3; c++) {
                    float mixed = blend_value(col[c], layer_col[c], layer->blend);
                    col[c] = clamp01(lerpf(col[c], mixed, clamp01(layer->intensity)));
                }
            }

            image.pixels[(y * image.width + x) * 4 + 0] = (unsigned char)(col[0] * 255.0f);
            image.pixels[(y * image.width + x) * 4 + 1] = (unsigned char)(col[1] * 255.0f);
            image.pixels[(y * image.width + x) * 4 + 2] = (unsigned char)(col[2] * 255.0f);
            image.pixels[(y * image.width + x) * 4 + 3] = 255;
        }
    }

    free(cellular);
    return image;
}

void teximage_free(TexImage *image) {
    if (!image) {
        return;
    }
    free(image->pixels);
    memset(image, 0, sizeof(*image));
}

SDL_Surface *texgen_generate_surface(const TexParams *params, int seed) {
    TexImage image = texgen_generate(params, seed);
    SDL_Surface *surface;

    if (!image.pixels) {
        return NULL;
    }

    surface = SDL_CreateRGBSurfaceWithFormat(0, image.width, image.height,
                                             32, SDL_PIXELFORMAT_RGBA32);
    if (surface) {
        memcpy(surface->pixels, image.pixels,
               (size_t)image.width * (size_t)image.height * 4u);
    }
    teximage_free(&image);
    if (surface && params->normal_map) {
        SDL_Surface *normal = texgen_normal_map(surface);
        if (normal) {
            SDL_FreeSurface(normal);
        }
    }
    return surface;
}

SDL_Texture *texgen_create_sdl(SDL_Renderer *ren, TexParams *params, int seed) {
    char cache_key[128];
    SDL_Texture *cached;
    SDL_Surface *surface;
    SDL_Texture *tex;

    if (!ren || !params) {
        return NULL;
    }

    snprintf(cache_key, sizeof(cache_key), "sdl:%s:%d", params->name, seed);
    cached = (SDL_Texture *)texcache_get(cache_key);
    if (cached) {
        return cached;
    }

    surface = texgen_generate_surface(params, seed);
    if (!surface) {
        return NULL;
    }

    tex = SDL_CreateTextureFromSurface(ren, surface);
    SDL_FreeSurface(surface);
    if (!tex) {
        return NULL;
    }
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    texcache_put(cache_key, tex);
    return tex;
}

unsigned int texgen_create_gl(TexParams *params, int seed) {
    char cache_key[128];
    GLuint texture = 0;
    GLuint *cached;
    TexImage image;
    GLuint *owned;

    if (!params) {
        return 0;
    }

    snprintf(cache_key, sizeof(cache_key), "gl:%s:%d", params->name, seed);
    cached = (GLuint *)texcache_get(cache_key);
    if (cached) {
        return *cached;
    }

    image = texgen_generate(params, seed);
    if (!image.pixels) {
        return 0;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    params->tiling ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    params->tiling ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, image.pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    teximage_free(&image);

    owned = malloc(sizeof(*owned));
    if (!owned) {
        return texture;
    }
    *owned = texture;
    texcache_put(cache_key, owned);
    return texture;
}

SDL_Surface *texgen_normal_map(SDL_Surface *heightmap) {
    SDL_Surface *normal;
    Uint32 *src;
    Uint32 *dst;

    if (!heightmap) {
        return NULL;
    }

    normal = SDL_CreateRGBSurfaceWithFormat(0, heightmap->w, heightmap->h,
                                            32, SDL_PIXELFORMAT_RGBA32);
    if (!normal) {
        return NULL;
    }

    SDL_LockSurface(heightmap);
    SDL_LockSurface(normal);
    src = (Uint32 *)heightmap->pixels;
    dst = (Uint32 *)normal->pixels;
    for (int y = 0; y < heightmap->h; y++) {
        for (int x = 0; x < heightmap->w; x++) {
            int xm = CLAMP(x - 1, 0, heightmap->w - 1);
            int x1 = CLAMP(x + 1, 0, heightmap->w - 1);
            int ym = CLAMP(y - 1, 0, heightmap->h - 1);
            int y1 = CLAMP(y + 1, 0, heightmap->h - 1);
            float hxm = (float)(src[y * heightmap->w + xm] & 0xFF) / 255.0f;
            float hxp = (float)(src[y * heightmap->w + x1] & 0xFF) / 255.0f;
            float hym = (float)(src[ym * heightmap->w + x] & 0xFF) / 255.0f;
            float hyp = (float)(src[y1 * heightmap->w + x] & 0xFF) / 255.0f;
            float sx = hxm - hxp;
            float sy = hym - hyp;
            float sz = 1.0f;
            float len = sqrtf(sx * sx + sy * sy + sz * sz);
            float nx = 0.5f;
            float ny = 0.5f;
            float nz = 1.0f;

            if (len > 0.0f) {
                nx = sx / len * 0.5f + 0.5f;
                ny = sy / len * 0.5f + 0.5f;
                nz = sz / len;
            }
            dst[y * heightmap->w + x] = RGBA((int)(clamp01(nx) * 255.0f),
                                             (int)(clamp01(ny) * 255.0f),
                                             (int)(clamp01(nz) * 255.0f),
                                             255);
        }
    }
    SDL_UnlockSurface(normal);
    SDL_UnlockSurface(heightmap);

    return normal;
}

static int l_register_texture(struct lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    TexParams params = texgen_parse_lua(L, 2);

    strncpy(params.name, name, TEXGEN_NAME_LEN - 1);
    params.name[TEXGEN_NAME_LEN - 1] = '\0';
    lua_pushboolean(L, texgen_register(name, &params));
    return 1;
}

int texgen_register_lua(struct lua_State *L) {
    lua_pushcfunction(L, l_register_texture);
    lua_setglobal(L, "register_texture");
    return 0;
}
