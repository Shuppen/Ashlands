#include "mesh.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float u, v;
} Vec2;

static void mesh_color_to_floats(uint32_t color,
                                 float *r, float *g, float *b, float *a) {
    *r = (float)RGBA_R(color) / 255.0f;
    *g = (float)RGBA_G(color) / 255.0f;
    *b = (float)RGBA_B(color) / 255.0f;
    *a = (float)RGBA_A(color) / 255.0f;
}

static bool mesh_builder_reserve(MeshBuilder *builder, size_t needed) {
    MeshVertex *grown;
    size_t new_capacity;

    if (needed <= builder->capacity) {
        return true;
    }

    new_capacity = builder->capacity ? builder->capacity : 256;
    while (new_capacity < needed) {
        if (new_capacity > SIZE_MAX / 2) {
            return false;
        }
        new_capacity *= 2;
    }

    grown = realloc(builder->vertices, new_capacity * sizeof(*grown));
    if (!grown) {
        return false;
    }

    builder->vertices = grown;
    builder->capacity = new_capacity;
    return true;
}

static bool mesh_builder_push_vertex(MeshBuilder *builder,
                                     float px, float py, float pz,
                                     float u, float v,
                                     float nx, float ny, float nz,
                                     uint32_t color) {
    MeshVertex *vert;

    if (!mesh_builder_reserve(builder, builder->count + 1)) {
        return false;
    }

    vert = &builder->vertices[builder->count++];
    vert->px = px;
    vert->py = py;
    vert->pz = pz;
    vert->u = u;
    vert->v = v;
    vert->nx = nx;
    vert->ny = ny;
    vert->nz = nz;
    mesh_color_to_floats(color, &vert->r, &vert->g, &vert->b, &vert->a);
    return true;
}

static bool mesh_builder_push_quad(MeshBuilder *builder,
                                   const float a[3], const float b[3],
                                   const float c[3], const float d[3],
                                   float nx, float ny, float nz,
                                   uint32_t color) {
    return mesh_builder_push_vertex(builder,
                                    a[0], a[1], a[2], 0.0f, 0.0f,
                                    nx, ny, nz, color) &&
           mesh_builder_push_vertex(builder,
                                    b[0], b[1], b[2], 1.0f, 0.0f,
                                    nx, ny, nz, color) &&
           mesh_builder_push_vertex(builder,
                                    c[0], c[1], c[2], 1.0f, 1.0f,
                                    nx, ny, nz, color) &&
           mesh_builder_push_vertex(builder,
                                    a[0], a[1], a[2], 0.0f, 0.0f,
                                    nx, ny, nz, color) &&
           mesh_builder_push_vertex(builder,
                                    c[0], c[1], c[2], 1.0f, 1.0f,
                                    nx, ny, nz, color) &&
           mesh_builder_push_vertex(builder,
                                    d[0], d[1], d[2], 0.0f, 1.0f,
                                    nx, ny, nz, color);
}

bool mesh_builder_init(MeshBuilder *builder, size_t initial_capacity) {
    memset(builder, 0, sizeof(*builder));
    if (!initial_capacity) {
        return true;
    }
    return mesh_builder_reserve(builder, initial_capacity);
}

void mesh_builder_reset(MeshBuilder *builder) {
    builder->count = 0;
}

void mesh_builder_free(MeshBuilder *builder) {
    if (!builder) {
        return;
    }

    free(builder->vertices);
    memset(builder, 0, sizeof(*builder));
}

bool mesh_builder_push_floor(MeshBuilder *builder, float x, float z,
                             uint32_t color) {
    const float a[3] = { x, 0.0f, z };
    const float b[3] = { x + 1.0f, 0.0f, z };
    const float c[3] = { x + 1.0f, 0.0f, z + 1.0f };
    const float d[3] = { x, 0.0f, z + 1.0f };

    return mesh_builder_push_quad(builder, a, b, c, d,
                                  0.0f, 1.0f, 0.0f, color);
}

bool mesh_builder_push_box(MeshBuilder *builder, float x, float z,
                           float height, uint32_t color) {
    const float nw0[3] = { x, 0.0f, z };
    const float ne0[3] = { x + 1.0f, 0.0f, z };
    const float se0[3] = { x + 1.0f, 0.0f, z + 1.0f };
    const float sw0[3] = { x, 0.0f, z + 1.0f };
    const float nw1[3] = { x, height, z };
    const float ne1[3] = { x + 1.0f, height, z };
    const float se1[3] = { x + 1.0f, height, z + 1.0f };
    const float sw1[3] = { x, height, z + 1.0f };

    return mesh_builder_push_quad(builder, sw1, se1, ne1, nw1,
                                  0.0f, 1.0f, 0.0f, color) &&
           mesh_builder_push_quad(builder, nw0, ne0, ne1, nw1,
                                  0.0f, 0.0f, -1.0f, color) &&
           mesh_builder_push_quad(builder, se0, sw0, sw1, se1,
                                  0.0f, 0.0f, 1.0f, color) &&
           mesh_builder_push_quad(builder, sw0, nw0, nw1, sw1,
                                  -1.0f, 0.0f, 0.0f, color) &&
           mesh_builder_push_quad(builder, ne0, se0, se1, ne1,
                                  1.0f, 0.0f, 0.0f, color);
}

bool mesh_builder_push_billboard(MeshBuilder *builder,
                                 float center_x, float base_y, float center_z,
                                 float width, float height,
                                 float right_x, float right_z,
                                 uint32_t color) {
    float half_w = width * 0.5f;
    float len = sqrtf(right_x * right_x + right_z * right_z);
    float nx = 0.0f;
    float nz = 1.0f;
    float a[3] = { center_x - right_x * half_w, base_y,
                   center_z - right_z * half_w };
    float b[3] = { center_x + right_x * half_w, base_y,
                   center_z + right_z * half_w };
    float c[3] = { center_x + right_x * half_w, base_y + height,
                   center_z + right_z * half_w };
    float d[3] = { center_x - right_x * half_w, base_y + height,
                   center_z - right_z * half_w };

    if (len > 0.0f) {
        nx = -right_z / len;
        nz = right_x / len;
    }

    return mesh_builder_push_quad(builder, a, b, c, d,
                                  nx, 0.0f, nz, color);
}

static int mesh_parse_face_vertex(const char *token,
                                  int *vi, int *ti, int *ni) {
    *vi = 0;
    *ti = 0;
    *ni = 0;

    if (sscanf(token, "%d/%d/%d", vi, ti, ni) == 3) {
        return 3;
    }
    if (sscanf(token, "%d//%d", vi, ni) == 2) {
        return 2;
    }
    if (sscanf(token, "%d/%d", vi, ti) == 2) {
        return 2;
    }
    if (sscanf(token, "%d", vi) == 1) {
        return 1;
    }

    return 0;
}

static bool mesh_push_obj_vertex(MeshBuilder *builder,
                                 const Vec3 *positions, size_t pos_count,
                                 const Vec2 *texcoords, size_t tex_count,
                                 const Vec3 *normals, size_t norm_count,
                                 int vi, int ti, int ni, uint32_t color) {
    Vec3 pos = { 0.0f, 0.0f, 0.0f };
    Vec2 tex = { 0.0f, 0.0f };
    Vec3 norm = { 0.0f, 1.0f, 0.0f };

    if (vi <= 0 || (size_t)vi > pos_count) {
        return false;
    }

    pos = positions[vi - 1];
    if (ti > 0 && (size_t)ti <= tex_count) {
        tex = texcoords[ti - 1];
    }
    if (ni > 0 && (size_t)ni <= norm_count) {
        norm = normals[ni - 1];
    }

    return mesh_builder_push_vertex(builder,
                                    pos.x, pos.y, pos.z,
                                    tex.u, tex.v,
                                    norm.x, norm.y, norm.z,
                                    color);
}

static bool mesh_push_face(MeshBuilder *builder,
                           const char *line,
                           const Vec3 *positions, size_t pos_count,
                           const Vec2 *texcoords, size_t tex_count,
                           const Vec3 *normals, size_t norm_count,
                           uint32_t color) {
    char face_buf[512];
    char *tokens[8];
    char *token;
    int token_count = 0;

    strncpy(face_buf, line, sizeof(face_buf) - 1);
    face_buf[sizeof(face_buf) - 1] = '\0';

    token = strtok(face_buf + 2, " \t");
    while (token && token_count < (int)ARRAY_LEN(tokens)) {
        tokens[token_count++] = token;
        token = strtok(NULL, " \t");
    }

    for (int i = 1; i + 1 < token_count; i++) {
        int vi, ti, ni;
        int vj, tj, nj;
        int vk, tk, nk;

        if (!mesh_parse_face_vertex(tokens[0], &vi, &ti, &ni) ||
            !mesh_parse_face_vertex(tokens[i], &vj, &tj, &nj) ||
            !mesh_parse_face_vertex(tokens[i + 1], &vk, &tk, &nk)) {
            return false;
        }

        if (!mesh_push_obj_vertex(builder, positions, pos_count,
                                  texcoords, tex_count,
                                  normals, norm_count,
                                  vi, ti, ni, color) ||
            !mesh_push_obj_vertex(builder, positions, pos_count,
                                  texcoords, tex_count,
                                  normals, norm_count,
                                  vj, tj, nj, color) ||
            !mesh_push_obj_vertex(builder, positions, pos_count,
                                  texcoords, tex_count,
                                  normals, norm_count,
                                  vk, tk, nk, color)) {
            return false;
        }
    }

    return true;
}

bool mesh_obj_load(const char *path, MeshData *mesh, uint32_t color) {
    FILE *fp;
    char line[512];
    MeshBuilder builder;
    Vec3 *positions = NULL;
    Vec2 *texcoords = NULL;
    Vec3 *normals = NULL;
    size_t pos_count = 0, pos_cap = 0;
    size_t tex_count = 0, tex_cap = 0;
    size_t norm_count = 0, norm_cap = 0;
    bool ok = false;

    if (!mesh) {
        return false;
    }

    memset(mesh, 0, sizeof(*mesh));
    if (!mesh_builder_init(&builder, 256)) {
        return false;
    }

    fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "[mesh] open %s failed\n", path);
        goto cleanup;
    }

    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);

        if (len > 0 && line[len - 1] == '\n') {
            line[--len] = '\0';
        }
        if (len > 0 && line[len - 1] == '\r') {
            line[--len] = '\0';
        }

        if (strncmp(line, "v ", 2) == 0) {
            Vec3 pos;

            if (sscanf(line + 2, "%f %f %f", &pos.x, &pos.y, &pos.z) == 3) {
                if (pos_count == pos_cap) {
                    size_t next = pos_cap ? pos_cap * 2 : 64;
                    Vec3 *grown = realloc(positions, next * sizeof(*grown));
                    if (!grown) {
                        goto cleanup;
                    }
                    positions = grown;
                    pos_cap = next;
                }
                positions[pos_count++] = pos;
            }
            continue;
        }

        if (strncmp(line, "vt ", 3) == 0) {
            Vec2 tex;

            if (sscanf(line + 3, "%f %f", &tex.u, &tex.v) == 2) {
                if (tex_count == tex_cap) {
                    size_t next = tex_cap ? tex_cap * 2 : 64;
                    Vec2 *grown = realloc(texcoords, next * sizeof(*grown));
                    if (!grown) {
                        goto cleanup;
                    }
                    texcoords = grown;
                    tex_cap = next;
                }
                texcoords[tex_count++] = tex;
            }
            continue;
        }

        if (strncmp(line, "vn ", 3) == 0) {
            Vec3 norm;

            if (sscanf(line + 3, "%f %f %f", &norm.x, &norm.y, &norm.z) == 3) {
                if (norm_count == norm_cap) {
                    size_t next = norm_cap ? norm_cap * 2 : 64;
                    Vec3 *grown = realloc(normals, next * sizeof(*grown));
                    if (!grown) {
                        goto cleanup;
                    }
                    normals = grown;
                    norm_cap = next;
                }
                normals[norm_count++] = norm;
            }
            continue;
        }

        if (line[0] == '#') {
            continue;
        }

        if (strncmp(line, "f ", 2) == 0 &&
            !mesh_push_face(&builder, line,
                            positions, pos_count,
                            texcoords, tex_count,
                            normals, norm_count,
                            color)) {
            goto cleanup;
        }
    }

    mesh->vertices = builder.vertices;
    mesh->vertex_count = builder.count;
    builder.vertices = NULL;
    builder.count = 0;
    builder.capacity = 0;
    ok = true;

cleanup:
    if (fp) {
        fclose(fp);
    }
    mesh_builder_free(&builder);
    free(positions);
    free(texcoords);
    free(normals);
    if (!ok) {
        mesh_data_free(mesh);
    }
    return ok;
}

void mesh_data_free(MeshData *mesh) {
    if (!mesh) {
        return;
    }

    free(mesh->vertices);
    mesh->vertices = NULL;
    mesh->vertex_count = 0;
}
