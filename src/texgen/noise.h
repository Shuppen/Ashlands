#ifndef ASHLANDS_NOISE_H
#define ASHLANDS_NOISE_H

#include <stddef.h>

float perlin2d(float x, float y, int seed);
float simplex2d(float x, float y, int seed);
float voronoi2d(float x, float y, int seed, float *edge_dist);
float noise_fbm(float (*fn)(float, float, int),
                float x, float y, int seed,
                int octaves, float persistence, float lacunarity);
void cellular_automata_generate(float *out, int width, int height,
                                int seed, int iterations);

#endif
