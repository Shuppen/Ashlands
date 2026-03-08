#ifndef ASHLANDS_MESH_H
#define ASHLANDS_MESH_H

#include <stdbool.h>
#include <stddef.h>

#include "../../include/ashlands.h"

typedef struct {
    float px, py, pz;
    float u, v;
    float nx, ny, nz;
    float r, g, b, a;
} MeshVertex;

typedef struct {
    MeshVertex *vertices;
    size_t count;
    size_t capacity;
} MeshBuilder;

typedef struct {
    MeshVertex *vertices;
    size_t vertex_count;
} MeshData;

bool mesh_builder_init(MeshBuilder *builder, size_t initial_capacity);
void mesh_builder_reset(MeshBuilder *builder);
void mesh_builder_free(MeshBuilder *builder);

bool mesh_builder_push_floor(MeshBuilder *builder, float x, float z,
                             uint32_t color);
bool mesh_builder_push_box(MeshBuilder *builder, float x, float z,
                           float height, uint32_t color);
bool mesh_builder_push_billboard(MeshBuilder *builder,
                                 float center_x, float base_y, float center_z,
                                 float width, float height,
                                 float right_x, float right_z,
                                 uint32_t color);

bool mesh_obj_load(const char *path, MeshData *mesh, uint32_t color);
void mesh_data_free(MeshData *mesh);

#endif
