/*
 * render_ascii.c — ASCII backend using SDL2_ttf
 * C11, MIT License
 *
 * Renders the world as monospaced glyphs on a grid.
 * Software renderer — no GPU needed, ~200 FPS on Celeron.
 */
#include "render.h"
#include "camera.h"
#include "../world.h"
#include "../ui.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* =========================================================
 * ASCII backend state
 * ========================================================= */
#define FONT_PATH "assets/fonts/DejaVuSansMono.ttf"
#define FONT_SIZE 16

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *ren;
    TTF_Font     *font;
    int           screen_w, screen_h;
    int           cell_w, cell_h;
} AsciiState;

static AsciiState s_ascii;

/* =========================================================
 * Tile → glyph + colours
 * ========================================================= */
typedef struct {
    char     glyph;
    uint32_t fg;
    uint32_t bg;
} TileGfx;

static const TileGfx TILE_GFX[TILE_COUNT] = {
    /* TILE_NONE       */ { ' ',  COL_BLACK,    COL_BLACK      },
    /* TILE_FLOOR      */ { '.',  COL_DARKGRAY, COL_BLACK      },
    /* TILE_WALL       */ { '#',  COL_GRAY,     COL_BLACK      },
    /* TILE_WATER      */ { '~',  COL_CYAN,     COL_BLACK      },
    /* TILE_LAVA       */ { '~',  COL_RED,      COL_BLACK      },
    /* TILE_DOOR_OPEN  */ { '/',  COL_BROWN,    COL_BLACK      },
    /* TILE_DOOR_SHUT  */ { '+',  COL_BROWN,    COL_BLACK      },
    /* TILE_STAIRS_UP  */ { '<',  COL_WHITE,    COL_BLACK      },
    /* TILE_STAIRS_DOWN */ { '>', COL_WHITE,    COL_BLACK      },
    /* TILE_ASH        */ { ',',  RGBA(140,130,120,255), COL_BLACK },
    /* TILE_GRASS      */ { '"',  COL_GREEN,    COL_BLACK      },
    /* TILE_SAND       */ { '.',  COL_YELLOW,   COL_BLACK      },
    /* TILE_TREE       */ { 'T',  COL_GREEN,    COL_BLACK      },
    /* TILE_RUBBLE     */ { '%',  COL_GRAY,     COL_BLACK      },
};

/* Dimmed colour for explored-but-not-visible tiles */
static uint32_t dim_color(uint32_t c) {
    uint8_t r = (uint8_t)(RGBA_R(c) / 3);
    uint8_t g = (uint8_t)(RGBA_G(c) / 3);
    uint8_t b = (uint8_t)(RGBA_B(c) / 3);
    return RGBA(r, g, b, 255);
}

/* =========================================================
 * Helper: draw a single glyph
 * ========================================================= */
static void draw_glyph(SDL_Renderer *ren, TTF_Font *font,
                        char ch, int px, int py,
                        uint32_t fg, uint32_t bg)
{
    /* Background */
    SDL_SetRenderDrawColor(ren,
        RGBA_R(bg), RGBA_G(bg), RGBA_B(bg), RGBA_A(bg));
    SDL_Rect bg_rect = { px, py, s_ascii.cell_w, s_ascii.cell_h };
    SDL_RenderFillRect(ren, &bg_rect);

    /* Glyph */
    if (ch == ' ') return;

    char buf[2] = { ch, '\0' };
    SDL_Color sdl_fg = { RGBA_R(fg), RGBA_G(fg), RGBA_B(fg), RGBA_A(fg) };
    SDL_Surface *surf = TTF_RenderText_Blended(font, buf, sdl_fg);
    if (!surf) return;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_FreeSurface(surf);
    if (!tex) return;

    int tw, th;
    SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
    SDL_Rect dst = {
        px + (s_ascii.cell_w - tw) / 2,
        py + (s_ascii.cell_h - th) / 2,
        tw, th
    };
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

/* =========================================================
 * Backend callbacks
 * ========================================================= */

static void ascii_init(SDL_Window *win, int width, int height) {
    s_ascii.window   = win;
    s_ascii.screen_w = width;
    s_ascii.screen_h = height;

    s_ascii.ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
    if (!s_ascii.ren) {
        fprintf(stderr, "[render_ascii] SDL_CreateRenderer: %s\n",
                SDL_GetError());
        return;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "[render_ascii] TTF_Init: %s\n", TTF_GetError());
        return;
    }

    s_ascii.font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    if (!s_ascii.font) {
        /* Fallback: try system path */
        s_ascii.font = TTF_OpenFont(
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
            FONT_SIZE);
    }
    if (!s_ascii.font) {
        fprintf(stderr, "[render_ascii] Cannot load font: %s\n",
                TTF_GetError());
        /* Use a dummy cell size */
        s_ascii.cell_w = 10;
        s_ascii.cell_h = 16;
        return;
    }

    TTF_SizeText(s_ascii.font, "@", &s_ascii.cell_w, &s_ascii.cell_h);
}

static void ascii_shutdown(void) {
    if (s_ascii.font) { TTF_CloseFont(s_ascii.font); s_ascii.font = NULL; }
    TTF_Quit();
    if (s_ascii.ren) { SDL_DestroyRenderer(s_ascii.ren); s_ascii.ren = NULL; }
}

static void ascii_begin_frame(void) {
    SDL_SetRenderDrawColor(s_ascii.ren, 0, 0, 0, 255);
    SDL_RenderClear(s_ascii.ren);
}

static void ascii_end_frame(void) {
    SDL_RenderPresent(s_ascii.ren);
}

static void ascii_render_map(const WorldState *ws, const Camera *cam) {
    if (!s_ascii.font) return;

    const GameMap *m = &ws->map;
    int x0, y0, x1, y1;
    camera_visible_rect(cam, &x0, &y0, &x1, &y1);

    for (int ty = y0; ty <= y1; ty++) {
        for (int tx = x0; tx <= x1; tx++) {
            if (!map_in_bounds(m, tx, ty)) continue;
            const Tile *t = &m->tiles[tx][ty];

            if (!t->explored) continue;

            TileType type = (TileType)CLAMP(t->type, 0, TILE_COUNT - 1);
            const TileGfx *gfx = &TILE_GFX[type];

            uint32_t fg = gfx->fg;
            uint32_t bg = gfx->bg;

            if (!t->visible) {
                fg = dim_color(fg);
                bg = dim_color(bg);
            }

            int sx, sy;
            camera_tile_to_screen(cam, tx, ty, &sx, &sy);
            draw_glyph(s_ascii.ren, s_ascii.font,
                       gfx->glyph, sx, sy, fg, bg);
        }
    }
}

static void ascii_render_entities(const WorldState *ws, const Camera *cam) {
    if (!s_ascii.font) return;

    const World   *w = ws->ecs;
    const GameMap *m = &ws->map;

    for (int id = 0; id < MAX_ENTITIES; id++) {
        if (!w->entities[id].active) continue;
        if (!entity_has((World*)w, id, COMP_POSITION | COMP_RENDER)) continue;

        const PositionComponent *pos = &w->positions[id];
        if (!map_in_bounds(m, pos->x, pos->y)) continue;
        if (!m->tiles[pos->x][pos->y].visible) continue;

        const RenderComponent *rc = &w->renders[id];
        int sx, sy;
        camera_tile_to_screen(cam, pos->x, pos->y, &sx, &sy);
        draw_glyph(s_ascii.ren, s_ascii.font,
                   rc->glyph, sx, sy, rc->fg_color, rc->bg_color);
    }
}

static void ascii_render_ui(const UIState *ui) {
    dialog_ui_render((UIState *)ui, ui->player_id);
    ui_render(ui, s_ascii.ren, s_ascii.font,
              s_ascii.screen_w, s_ascii.screen_h);
}

static void ascii_on_resize(int width, int height) {
    s_ascii.screen_w = width;
    s_ascii.screen_h = height;
}

/* =========================================================
 * Factory
 * ========================================================= */

static Renderer s_ascii_renderer = {
    .mode             = RENDER_ASCII,
    .init             = ascii_init,
    .shutdown         = ascii_shutdown,
    .begin_frame      = ascii_begin_frame,
    .end_frame        = ascii_end_frame,
    .render_map       = ascii_render_map,
    .render_entities  = ascii_render_entities,
    .render_ui        = ascii_render_ui,
    .on_resize        = ascii_on_resize,
};

Renderer *render_ascii_create(void) {
    return &s_ascii_renderer;
}
