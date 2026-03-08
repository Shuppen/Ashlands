#include "render_3d_scene.h"

#include "../world.h"

#include <math.h>
#include <string.h>

#define WALL_HEIGHT 1.5f
#define R3D_VIEW_RADIUS 26

static bool tile_has_floor(TileType type) {
    return type != TILE_NONE && type != TILE_WALL && type != TILE_TREE &&
           type != TILE_DOOR_SHUT;
}

static bool tile_has_wall(TileType type) {
    return type == TILE_WALL || type == TILE_TREE || type == TILE_DOOR_SHUT;
}

static uint32_t tile_color(TileType type, bool visible) {
    uint32_t color;

    switch (type) {
    case TILE_WALL:        color = RGBA(110, 106, 102, 255); break;
    case TILE_WATER:       color = RGBA(68, 92, 138, 255); break;
    case TILE_LAVA:        color = RGBA(220, 110, 36, 255); break;
    case TILE_DOOR_OPEN:   color = RGBA(116, 84, 54, 255); break;
    case TILE_DOOR_SHUT:   color = RGBA(104, 74, 46, 255); break;
    case TILE_ASH:         color = RGBA(126, 118, 108, 255); break;
    case TILE_GRASS:       color = RGBA(92, 102, 68, 255); break;
    case TILE_SAND:        color = RGBA(162, 146, 102, 255); break;
    case TILE_TREE:        color = RGBA(72, 94, 66, 255); break;
    case TILE_RUBBLE:      color = RGBA(96, 92, 88, 255); break;
    case TILE_FLOOR:
    default:               color = RGBA(92, 88, 84, 255); break;
    }

    if (!visible) {
        color = RGBA(RGBA_R(color) / 2,
                     RGBA_G(color) / 2,
                     RGBA_B(color) / 2,
                     255);
    }
    return color;
}

static bool render_3d_scene_reset_builders(Render3dScene *scene) {
    for (int i = 0; i < TILE_COUNT; i++) {
        mesh_builder_reset(&scene->world_builders[i]);
    }
    mesh_builder_reset(&scene->entity_builder);
    scene->point_light_count = 0;
    return true;
}

static void render_3d_scene_collect_lights(Render3dScene *scene,
                                           const WorldState *ws,
                                           int x0, int y0, int x1, int y1) {
    const World *w = ws->ecs;

    if (w && w->player_id >= 0 && entity_has((World *)w, w->player_id, COMP_POSITION)) {
        const PositionComponent *player = &w->positions[w->player_id];
        scene->point_lights[scene->point_light_count].position[0] = (float)player->x + 0.5f;
        scene->point_lights[scene->point_light_count].position[1] = 0.8f;
        scene->point_lights[scene->point_light_count].position[2] = (float)player->y + 0.5f;
        scene->point_lights[scene->point_light_count].color[0] = 0.95f;
        scene->point_lights[scene->point_light_count].color[1] = 0.72f;
        scene->point_lights[scene->point_light_count].color[2] = 0.42f;
        scene->point_lights[scene->point_light_count].radius = 6.0f;
        scene->point_light_count++;
    }

    for (int ty = y0; ty <= y1 && scene->point_light_count < 4; ty++) {
        for (int tx = x0; tx <= x1 && scene->point_light_count < 4; tx++) {
            const Tile *tile;

            if (!map_in_bounds(&ws->map, tx, ty)) {
                continue;
            }

            tile = &ws->map.tiles[tx][ty];
            if (!tile->visible) {
                continue;
            }

            if (tile->type == TILE_LAVA) {
                Render3dPointLight *light = &scene->point_lights[scene->point_light_count++];
                light->position[0] = (float)tx + 0.5f;
                light->position[1] = 0.3f;
                light->position[2] = (float)ty + 0.5f;
                light->color[0] = 1.00f;
                light->color[1] = 0.36f;
                light->color[2] = 0.10f;
                light->radius = 5.5f;
            }
        }
    }
}

bool render_3d_scene_init(Render3dScene *scene) {
    memset(scene, 0, sizeof(*scene));
    for (int i = 0; i < TILE_COUNT; i++) {
        if (!mesh_builder_init(&scene->world_builders[i], 4096)) {
            render_3d_scene_shutdown(scene);
            return false;
        }
    }
    if (!mesh_builder_init(&scene->entity_builder, 2048)) {
        render_3d_scene_shutdown(scene);
        return false;
    }
    return true;
}

void render_3d_scene_shutdown(Render3dScene *scene) {
    for (int i = 0; i < TILE_COUNT; i++) {
        mesh_builder_free(&scene->world_builders[i]);
    }
    mesh_builder_free(&scene->entity_builder);
    memset(scene, 0, sizeof(*scene));
}

void render_3d_scene_prepare(Render3dScene *scene, const Camera *cam,
                             int screen_w, int screen_h) {
    Mat4 vp;
    float aspect;
    float forward_x;
    float forward_z;
    float inv_len;

    if (!scene->follow_initialized) {
        scene->follow_x = cam->x;
        scene->follow_y = cam->y;
        scene->follow_initialized = true;
    }

    scene->follow_x += (cam->x - scene->follow_x) * 0.18f;
    scene->follow_y += (cam->y - scene->follow_y) * 0.18f;
    scene->center[0] = scene->follow_x + 0.5f;
    scene->center[1] = 0.35f;
    scene->center[2] = scene->follow_y + 0.5f;
    scene->eye[0] = scene->center[0] + 7.0f;
    scene->eye[1] = 8.0f;
    scene->eye[2] = scene->center[2] + 6.0f;

    forward_x = scene->center[0] - scene->eye[0];
    forward_z = scene->center[2] - scene->eye[2];
    inv_len = 1.0f / MAX(sqrtf(forward_x * forward_x + forward_z * forward_z), 0.001f);
    scene->billboard_right_x = -forward_z * inv_len;
    scene->billboard_right_z = forward_x * inv_len;

    aspect = (float)screen_w / (float)MAX(screen_h, 1);
    mat4_identity(&scene->model);
    mat4_lookat(&scene->view,
                scene->eye[0], scene->eye[1], scene->eye[2],
                scene->center[0], scene->center[1], scene->center[2],
                0.0f, 1.0f, 0.0f);
    mat4_perspective(&scene->proj, 45.0f, aspect, 0.1f, 64.0f);
    mat4_multiply(&vp, &scene->proj, &scene->view);
    mat4_multiply(&scene->mvp, &vp, &scene->model);
}

bool render_3d_scene_build_world(Render3dScene *scene,
                                 const WorldState *ws,
                                 const Camera *cam) {
    int x0, y0, x1, y1;

    render_3d_scene_reset_builders(scene);
    camera_visible_rect(cam, &x0, &y0, &x1, &y1);
    x0 -= 4;
    y0 -= 4;
    x1 += 4;
    y1 += 4;

    render_3d_scene_collect_lights(scene, ws, x0, y0, x1, y1);
    for (int ty = y0; ty <= y1; ty++) {
        for (int tx = x0; tx <= x1; tx++) {
            const Tile *tile;
            uint32_t color;
            int dx;
            int dy;

            if (!map_in_bounds(&ws->map, tx, ty)) {
                continue;
            }

            dx = tx - (int)scene->center[0];
            dy = ty - (int)scene->center[2];
            if (dx * dx + dy * dy > R3D_VIEW_RADIUS * R3D_VIEW_RADIUS) {
                continue;
            }

            tile = &ws->map.tiles[tx][ty];
            if (!tile->explored) {
                continue;
            }

            color = tile_color(tile->type, tile->visible);
            if (tile_has_floor(tile->type) &&
                !mesh_builder_push_floor(&scene->world_builders[tile->type],
                                         (float)tx, (float)ty, color)) {
                return false;
            }
            if (tile_has_wall(tile->type) &&
                !mesh_builder_push_box(&scene->world_builders[tile->type],
                                       (float)tx, (float)ty,
                                       WALL_HEIGHT, color)) {
                return false;
            }
        }
    }

    return true;
}

bool render_3d_scene_build_entities(Render3dScene *scene,
                                    const WorldState *ws) {
    const World *w = ws->ecs;

    mesh_builder_reset(&scene->entity_builder);
    for (int id = 0; id < MAX_ENTITIES; id++) {
        const PositionComponent *pos;
        const RenderComponent *rc;
        float width = 0.72f;
        float height = 1.25f;

        if (!w->entities[id].active) {
            continue;
        }
        if (!entity_has((World *)w, id, COMP_POSITION | COMP_RENDER)) {
            continue;
        }

        pos = &w->positions[id];
        if (!map_in_bounds(&ws->map, pos->x, pos->y) ||
            !ws->map.tiles[pos->x][pos->y].visible) {
            continue;
        }

        rc = &w->renders[id];
        if (id == w->player_id) {
            width = 0.82f;
            height = 1.45f;
        } else if (entity_has((World *)w, id, COMP_ITEM)) {
            width = 0.55f;
            height = 0.70f;
        }

        if (!mesh_builder_push_billboard(&scene->entity_builder,
                                         (float)pos->x + 0.5f,
                                         0.0f,
                                         (float)pos->y + 0.5f,
                                         width,
                                         height,
                                         scene->billboard_right_x,
                                         scene->billboard_right_z,
                                         rc->fg_color)) {
            return false;
        }
    }

    return true;
}
