/*
 * render.h — Abstract renderer interface
 * C11, MIT License
 *
 * The game logic never knows how things are drawn.
 * Switch render backend at runtime via g_renderer.
 */
#ifndef ASHLANDS_RENDER_H
#define ASHLANDS_RENDER_H

#include <SDL2/SDL.h>
#include "../../include/ashlands.h"

/* Forward declarations */
typedef struct WorldState WorldState;
typedef struct UIState    UIState;
typedef struct Camera     Camera;

/* =========================================================
 * Render modes
 * ========================================================= */
typedef enum {
    RENDER_ASCII       = 0,  /* Classic roguelike: @#.~  */
    RENDER_TILES_2D    = 1,  /* 2D sprite tiles           */
    RENDER_ISO_25D     = 2,  /* 2.5D isometric            */
    RENDER_LOWPOLY_3D  = 3,  /* Low-poly OpenGL 2.0       */
    RENDER_MODE_COUNT
} RenderMode;

/* =========================================================
 * Renderer interface (function-pointer table)
 * ========================================================= */
typedef struct Renderer {
    RenderMode mode;

    void (*init)(SDL_Window *win, int width, int height);
    void (*shutdown)(void);

    void (*begin_frame)(void);
    void (*end_frame)(void);

    void (*render_map)(const WorldState *ws, const Camera *cam);
    void (*render_entities)(const WorldState *ws, const Camera *cam);
    void (*render_ui)(const UIState *ui);

    /* Optional: resize callback */
    void (*on_resize)(int width, int height);
} Renderer;

/* =========================================================
 * Global renderer — set by engine, used everywhere
 * ========================================================= */
extern Renderer *g_renderer;

/* =========================================================
 * Renderer factories (one per backend)
 * ========================================================= */
Renderer *renderer_create(RenderMode mode);
Renderer *render_ascii_create(void);
Renderer *render_tiles_create(void);   /* stub for Phase 2 */
Renderer *render_iso_create(void);     /* stub for Phase 2 */
Renderer *render_3d_create(void);      /* stub for Phase 3 */

void renderer_destroy(Renderer *r);

/* =========================================================
 * Convenience: switch active renderer
 * ========================================================= */
void renderer_set(Renderer *r, SDL_Window *win, int w, int h);

#endif /* ASHLANDS_RENDER_H */
