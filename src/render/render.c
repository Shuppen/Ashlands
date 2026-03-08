#include "render.h"

Renderer *g_renderer = NULL;

Renderer *renderer_create(RenderMode mode) {
    switch (mode) {
    case RENDER_ASCII:
        return render_ascii_create();
    case RENDER_TILES_2D:
        return render_tiles_create();
    case RENDER_ISO_25D:
        return render_iso_create();
    case RENDER_LOWPOLY_3D:
        return render_3d_create();
    case RENDER_MODE_COUNT:
    default:
        return render_ascii_create();
    }
}

Renderer *render_tiles_create(void) {
    return render_ascii_create();
}

Renderer *render_iso_create(void) {
    return render_ascii_create();
}

void renderer_destroy(Renderer *r) {
    (void)r;
}

void renderer_set(Renderer *r, SDL_Window *win, int w, int h) {
    if (g_renderer && g_renderer->shutdown) {
        g_renderer->shutdown();
    }

    g_renderer = r;
    if (g_renderer && g_renderer->init) {
        g_renderer->init(win, w, h);
    }
}
