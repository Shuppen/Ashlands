#ifndef ASHLANDS_RENDER_3D_SCENE_H
#define ASHLANDS_RENDER_3D_SCENE_H

#include <stdbool.h>

#include "camera.h"
#include "mat4.h"
#include "mesh.h"

typedef struct WorldState WorldState;

typedef struct {
    float position[3];
    float color[3];
    float radius;
} Render3dPointLight;

typedef struct {
    MeshBuilder world_builders[TILE_COUNT];
    MeshBuilder entity_builder;
    Mat4 model;
    Mat4 view;
    Mat4 proj;
    Mat4 mvp;
    float eye[3];
    float center[3];
    float billboard_right_x;
    float billboard_right_z;
    float follow_x;
    float follow_y;
    Render3dPointLight point_lights[4];
    int point_light_count;
    bool follow_initialized;
} Render3dScene;

bool render_3d_scene_init(Render3dScene *scene);
void render_3d_scene_shutdown(Render3dScene *scene);
void render_3d_scene_prepare(Render3dScene *scene, const Camera *cam,
                             int screen_w, int screen_h);
bool render_3d_scene_build_world(Render3dScene *scene,
                                 const WorldState *ws,
                                 const Camera *cam);
bool render_3d_scene_build_entities(Render3dScene *scene,
                                    const WorldState *ws);

#endif
