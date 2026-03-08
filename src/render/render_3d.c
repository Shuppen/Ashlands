#include "render.h"

#include "shader.h"
#include "render_3d_scene.h"
#include "../texgen/texgen.h"
#include "../world.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    SDL_Window   *window;
    SDL_GLContext gl_context;
    int           screen_w;
    int           screen_h;
    GLuint        world_vbos[TILE_COUNT];
    GLuint        entity_vbo;
    GLuint        white_texture;
    GLuint        tile_textures[TILE_COUNT];
    ShaderProgram world_shader;
    GLint         u_mvp;
    GLint         u_model;
    GLint         u_texture;
    GLint         u_ambient_color;
    GLint         u_light_dir;
    GLint         u_light_color;
    GLint         u_fog_density;
    GLint         u_fog_color;
    GLint         u_camera_pos;
    Render3dScene scene;
    bool          scene_ready;
    bool          ready;
} Render3dState;

static Render3dState s_3d;

static void render_3d_shutdown(void);

static GLuint render_3d_tile_texture(TileType type) {
    char tex_name[TEXGEN_NAME_LEN];
    const TexParams *params;

    if (type < 0 || type >= TILE_COUNT) {
        return s_3d.white_texture;
    }
    if (s_3d.tile_textures[type]) {
        return s_3d.tile_textures[type];
    }
    if (!texgen_lookup_tile_texture(type, tex_name, sizeof(tex_name))) {
        return s_3d.white_texture;
    }

    params = texgen_get(tex_name);
    if (!params) {
        return s_3d.white_texture;
    }

    s_3d.tile_textures[type] = texgen_create_gl((TexParams *)params, type * 97);
    return s_3d.tile_textures[type] ? s_3d.tile_textures[type] : s_3d.white_texture;
}

static void render_3d_apply_viewport(void) {
    glViewport(0, 0, s_3d.screen_w, s_3d.screen_h);
}

static void render_3d_compute_lighting(const WorldState *ws,
                                       float ambient[3], float fog[3]) {
    float t = (float)ws->time.hour / 24.0f;
    float dusk = 0.35f + 0.35f * sinf(t * 6.2831853f - 1.5707963f);
    float daylight = CLAMP(dusk, 0.15f, 0.7f);

    ambient[0] = 0.18f + daylight * 0.28f;
    ambient[1] = 0.17f + daylight * 0.24f;
    ambient[2] = 0.16f + daylight * 0.20f;
    fog[0] = 0.10f + daylight * 0.10f;
    fog[1] = 0.09f + daylight * 0.08f;
    fog[2] = 0.08f + daylight * 0.06f;
}

static void render_3d_bind_common_uniforms(const WorldState *ws) {
    float ambient[3];
    float fog[3];
    static const float light_dir[3] = { -0.45f, -1.0f, -0.35f };
    static const float light_color[3] = { 0.72f, 0.68f, 0.62f };

    render_3d_compute_lighting(ws, ambient, fog);
    glUniformMatrix4fv(s_3d.u_mvp, 1, GL_FALSE, s_3d.scene.mvp.m);
    glUniformMatrix4fv(s_3d.u_model, 1, GL_FALSE, s_3d.scene.model.m);
    glUniform1i(s_3d.u_texture, 0);
    glUniform3f(s_3d.u_ambient_color, ambient[0], ambient[1], ambient[2]);
    glUniform3f(s_3d.u_light_dir, light_dir[0], light_dir[1], light_dir[2]);
    glUniform3f(s_3d.u_light_color,
                light_color[0], light_color[1], light_color[2]);
    glUniform1f(s_3d.u_fog_density, 0.055f);
    glUniform3f(s_3d.u_fog_color, fog[0], fog[1], fog[2]);
    glUniform3f(s_3d.u_camera_pos,
                s_3d.scene.eye[0], s_3d.scene.eye[1], s_3d.scene.eye[2]);
}

static void render_3d_draw_builder(const WorldState *ws,
                                   const MeshBuilder *builder,
                                   GLuint vbo,
                                   GLuint texture,
                                   bool enable_blend) {
    if (!builder->count) {
        return;
    }

    shader_program_use(&s_3d.world_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture ? texture : s_3d.white_texture);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 builder->count * sizeof(MeshVertex),
                 builder->vertices,
                 GL_DYNAMIC_DRAW);
    render_3d_bind_common_uniforms(ws);

    if (enable_blend) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glEnableVertexAttribArray(SHADER_ATTRIB_POSITION);
    glEnableVertexAttribArray(SHADER_ATTRIB_TEXCOORD);
    glEnableVertexAttribArray(SHADER_ATTRIB_NORMAL);
    glEnableVertexAttribArray(SHADER_ATTRIB_COLOR);
    glVertexAttribPointer(SHADER_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE,
                          sizeof(MeshVertex), (const void *)offsetof(MeshVertex, px));
    glVertexAttribPointer(SHADER_ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                          sizeof(MeshVertex), (const void *)offsetof(MeshVertex, u));
    glVertexAttribPointer(SHADER_ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE,
                          sizeof(MeshVertex), (const void *)offsetof(MeshVertex, nx));
    glVertexAttribPointer(SHADER_ATTRIB_COLOR, 4, GL_FLOAT, GL_FALSE,
                          sizeof(MeshVertex), (const void *)offsetof(MeshVertex, r));
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)builder->count);
    glDisableVertexAttribArray(SHADER_ATTRIB_POSITION);
    glDisableVertexAttribArray(SHADER_ATTRIB_TEXCOORD);
    glDisableVertexAttribArray(SHADER_ATTRIB_NORMAL);
    glDisableVertexAttribArray(SHADER_ATTRIB_COLOR);

    if (enable_blend) {
        glDisable(GL_BLEND);
    }
}

static void render_3d_create_white_texture(void) {
    static const unsigned char pixel[4] = { 255, 255, 255, 255 };

    glGenTextures(1, &s_3d.white_texture);
    glBindTexture(GL_TEXTURE_2D, s_3d.white_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void render_3d_init(SDL_Window *win, int width, int height) {
    memset(&s_3d, 0, sizeof(s_3d));
    s_3d.window = win;
    s_3d.screen_w = width;
    s_3d.screen_h = height;

    if (!render_3d_scene_init(&s_3d.scene)) {
        fprintf(stderr, "[render_3d] scene init failed\n");
        render_3d_shutdown();
        return;
    }

    s_3d.gl_context = SDL_GL_CreateContext(win);
    if (!s_3d.gl_context) {
        fprintf(stderr, "[render_3d] SDL_GL_CreateContext: %s\n", SDL_GetError());
        render_3d_shutdown();
        return;
    }

    if (SDL_GL_MakeCurrent(win, s_3d.gl_context) != 0) {
        fprintf(stderr, "[render_3d] SDL_GL_MakeCurrent: %s\n", SDL_GetError());
        render_3d_shutdown();
        return;
    }

    render_3d_apply_viewport();
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
#ifdef PLATFORM_WEB
    glClearDepthf(1.0f);
#else
    glClearDepth(1.0);
#endif
    glClearColor(0.08f, 0.07f, 0.06f, 1.0f);

    if (!shader_program_load_files(&s_3d.world_shader,
                                   "assets/shaders/world.vert",
                                   "assets/shaders/world.frag")) {
        fprintf(stderr, "[render_3d] world shader load failed\n");
        render_3d_shutdown();
        return;
    }

    s_3d.u_mvp = shader_program_uniform(&s_3d.world_shader, "u_mvp");
    s_3d.u_model = shader_program_uniform(&s_3d.world_shader, "u_model");
    s_3d.u_texture = shader_program_uniform(&s_3d.world_shader, "u_texture");
    s_3d.u_ambient_color = shader_program_uniform(&s_3d.world_shader, "u_ambient_color");
    s_3d.u_light_dir = shader_program_uniform(&s_3d.world_shader, "u_light_dir");
    s_3d.u_light_color = shader_program_uniform(&s_3d.world_shader, "u_light_color");
    s_3d.u_fog_density = shader_program_uniform(&s_3d.world_shader, "u_fog_density");
    s_3d.u_fog_color = shader_program_uniform(&s_3d.world_shader, "u_fog_color");
    s_3d.u_camera_pos = shader_program_uniform(&s_3d.world_shader, "u_camera_pos");

    glGenBuffers(TILE_COUNT, s_3d.world_vbos);
    glGenBuffers(1, &s_3d.entity_vbo);
    render_3d_create_white_texture();
    s_3d.ready = true;
}

static void render_3d_shutdown(void) {
    shader_program_use(NULL);
    glDeleteBuffers(TILE_COUNT, s_3d.world_vbos);
    if (s_3d.entity_vbo) {
        glDeleteBuffers(1, &s_3d.entity_vbo);
    }
    for (int i = 0; i < TILE_COUNT; i++) {
        if (s_3d.tile_textures[i]) {
            glDeleteTextures(1, &s_3d.tile_textures[i]);
        }
    }
    if (s_3d.white_texture) {
        glDeleteTextures(1, &s_3d.white_texture);
    }
    shader_program_destroy(&s_3d.world_shader);
    if (s_3d.gl_context) {
        SDL_GL_DeleteContext(s_3d.gl_context);
    }
    render_3d_scene_shutdown(&s_3d.scene);
    memset(&s_3d, 0, sizeof(s_3d));
}

static void render_3d_begin_frame(void) {
    s_3d.scene_ready = false;
    if (!s_3d.ready) {
        return;
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void render_3d_end_frame(void) {
    if (!s_3d.ready || !s_3d.window) {
        return;
    }
    SDL_GL_SwapWindow(s_3d.window);
}

static void render_3d_render_map(const WorldState *ws, const Camera *cam) {
    if (!s_3d.ready) {
        return;
    }

    render_3d_scene_prepare(&s_3d.scene, cam, s_3d.screen_w, s_3d.screen_h);
    if (!render_3d_scene_build_world(&s_3d.scene, ws, cam)) {
        fprintf(stderr, "[render_3d] world scene build failed\n");
        return;
    }

    for (int type = 0; type < TILE_COUNT; type++) {
        render_3d_draw_builder(ws,
                               &s_3d.scene.world_builders[type],
                               s_3d.world_vbos[type],
                               render_3d_tile_texture((TileType)type),
                               false);
    }
    s_3d.scene_ready = true;
}

static void render_3d_render_entities(const WorldState *ws, const Camera *cam) {
    if (!s_3d.ready) {
        return;
    }
    if (!s_3d.scene_ready) {
        render_3d_scene_prepare(&s_3d.scene, cam, s_3d.screen_w, s_3d.screen_h);
    }

    if (!render_3d_scene_build_entities(&s_3d.scene, ws)) {
        fprintf(stderr, "[render_3d] entity scene build failed\n");
        return;
    }

    render_3d_draw_builder(ws,
                           &s_3d.scene.entity_builder,
                           s_3d.entity_vbo,
                           s_3d.white_texture,
                           true);
}

static void render_3d_render_ui(const UIState *ui) {
    (void)ui;
}

static void render_3d_on_resize(int width, int height) {
    s_3d.screen_w = width;
    s_3d.screen_h = height;
    if (s_3d.ready) {
        render_3d_apply_viewport();
    }
}

static Renderer s_3d_renderer = {
    .mode            = RENDER_LOWPOLY_3D,
    .init            = render_3d_init,
    .shutdown        = render_3d_shutdown,
    .begin_frame     = render_3d_begin_frame,
    .end_frame       = render_3d_end_frame,
    .render_map      = render_3d_render_map,
    .render_entities = render_3d_render_entities,
    .render_ui       = render_3d_render_ui,
    .on_resize       = render_3d_on_resize,
};

Renderer *render_3d_create(void) {
    return &s_3d_renderer;
}
