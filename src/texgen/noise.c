#include "noise.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint32_t noise_hash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

static uint32_t noise_hash2(int x, int y, int seed) {
    uint32_t h = (uint32_t)seed;
    h ^= noise_hash((uint32_t)(x * 374761393));
    h ^= noise_hash((uint32_t)(y * 668265263));
    return noise_hash(h);
}

static float noise_rand01(int x, int y, int seed) {
    return (float)(noise_hash2(x, y, seed) & 0x00ffffffu) / 16777215.0f;
}

static float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static float grad2(int ix, int iy, int seed, float x, float y) {
    static const float grads[8][2] = {
        { 1.0f, 0.0f }, {-1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, -1.0f },
        { 0.7071f, 0.7071f }, {-0.7071f, 0.7071f },
        { 0.7071f, -0.7071f }, {-0.7071f, -0.7071f }
    };
    uint32_t h = noise_hash2(ix, iy, seed);
    const float *g = grads[h & 7u];
    return g[0] * x + g[1] * y;
}

float perlin2d(float x, float y, int seed) {
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    float sx = x - (float)x0;
    float sy = y - (float)y0;
    float n0 = grad2(x0, y0, seed, x - (float)x0, y - (float)y0);
    float n1 = grad2(x1, y0, seed, x - (float)x1, y - (float)y0);
    float ix0 = lerpf(n0, n1, fade(sx));
    float n2 = grad2(x0, y1, seed, x - (float)x0, y - (float)y1);
    float n3 = grad2(x1, y1, seed, x - (float)x1, y - (float)y1);
    float ix1 = lerpf(n2, n3, fade(sx));

    return lerpf(ix0, ix1, fade(sy));
}

float simplex2d(float x, float y, int seed) {
    const float f2 = 0.36602540378f;
    const float g2 = 0.2113248654f;
    float s = (x + y) * f2;
    int i = (int)floorf(x + s);
    int j = (int)floorf(y + s);
    float t = (float)(i + j) * g2;
    float x0 = x - ((float)i - t);
    float y0 = y - ((float)j - t);
    int i1 = x0 > y0 ? 1 : 0;
    int j1 = x0 > y0 ? 0 : 1;
    float x1 = x0 - (float)i1 + g2;
    float y1 = y0 - (float)j1 + g2;
    float x2 = x0 - 1.0f + 2.0f * g2;
    float y2 = y0 - 1.0f + 2.0f * g2;
    float n0 = 0.0f;
    float n1 = 0.0f;
    float n2 = 0.0f;
    float t0 = 0.5f - x0 * x0 - y0 * y0;
    float t1 = 0.5f - x1 * x1 - y1 * y1;
    float t2 = 0.5f - x2 * x2 - y2 * y2;

    if (t0 > 0.0f) {
        t0 *= t0;
        n0 = t0 * t0 * grad2(i, j, seed, x0, y0);
    }
    if (t1 > 0.0f) {
        t1 *= t1;
        n1 = t1 * t1 * grad2(i + i1, j + j1, seed, x1, y1);
    }
    if (t2 > 0.0f) {
        t2 *= t2;
        n2 = t2 * t2 * grad2(i + 1, j + 1, seed, x2, y2);
    }

    return 40.0f * (n0 + n1 + n2);
}

float voronoi2d(float x, float y, int seed, float *edge_dist) {
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    float best = 1e9f;
    float second = 1e9f;

    for (int oy = -1; oy <= 1; oy++) {
        for (int ox = -1; ox <= 1; ox++) {
            int cx = ix + ox;
            int cy = iy + oy;
            float fx = (float)cx + noise_rand01(cx, cy, seed);
            float fy = (float)cy + noise_rand01(cx, cy, seed ^ 0x5bd1e995);
            float dx = fx - x;
            float dy = fy - y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist < best) {
                second = best;
                best = dist;
            } else if (dist < second) {
                second = dist;
            }
        }
    }

    if (edge_dist) {
        *edge_dist = second - best;
    }
    return best;
}

float noise_fbm(float (*fn)(float, float, int),
                float x, float y, int seed,
                int octaves, float persistence, float lacunarity) {
    float sum = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float total_amp = 0.0f;

    if (octaves < 1) {
        octaves = 1;
    }

    for (int i = 0; i < octaves; i++) {
        sum += fn(x * frequency, y * frequency, seed + i * 101) * amplitude;
        total_amp += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return total_amp > 0.0f ? sum / total_amp : 0.0f;
}

void cellular_automata_generate(float *out, int width, int height,
                                int seed, int iterations) {
    float *tmp;

    if (!out || width <= 0 || height <= 0) {
        return;
    }

    tmp = malloc((size_t)width * (size_t)height * sizeof(*tmp));
    if (!tmp) {
        return;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            out[y * width + x] = noise_rand01(x, y, seed) > 0.54f ? 1.0f : 0.0f;
        }
    }

    for (int iter = 0; iter < iterations; iter++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int count = 0;

                for (int oy = -1; oy <= 1; oy++) {
                    for (int ox = -1; ox <= 1; ox++) {
                        int nx = x + ox;
                        int ny = y + oy;
                        if (ox == 0 && oy == 0) {
                            continue;
                        }
                        if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
                            count++;
                        } else if (out[ny * width + nx] > 0.5f) {
                            count++;
                        }
                    }
                }

                tmp[y * width + x] = count >= 5 ? 1.0f : 0.0f;
            }
        }
        memcpy(out, tmp, (size_t)width * (size_t)height * sizeof(*out));
    }

    free(tmp);
}
