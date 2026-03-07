/*
 * ui.c — In-game user interface
 * C11, MIT License
 */
#include "ui.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <stdio.h>

void ui_state_init(UIState *ui, int player_id) {
    memset(ui, 0, sizeof(UIState));
    ui->player_id = player_id;
}

void ui_log(UIState *ui, const char *text, uint32_t color) {
    int idx = ui->log.head % UI_LOG_LINES;
    strncpy(ui->log.lines[idx], text, UI_LOG_MAX_LEN - 1);
    ui->log.lines[idx][UI_LOG_MAX_LEN - 1] = '\0';
    ui->log.colors[idx] = color;
    ui->log.head++;
    if (ui->log.count < UI_LOG_LINES) ui->log.count++;
}

/* =========================================================
 * Helper: render one line of text
 * ========================================================= */
static void render_text(SDL_Renderer *ren, TTF_Font *font,
                         const char *text, int x, int y, uint32_t color)
{
    if (!font || !text || !text[0]) return;
    SDL_Color c = {
        RGBA_R(color), RGBA_G(color), RGBA_B(color), RGBA_A(color)
    };
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, c);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    if (tex) {
        int tw, th;
        SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
        SDL_Rect dst = { x, y, tw, th };
        SDL_RenderCopy(ren, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void ui_render(const UIState *ui, void *sdl_renderer, void *ttf_font,
               int screen_w, int screen_h)
{
    SDL_Renderer *ren  = (SDL_Renderer *)sdl_renderer;
    TTF_Font     *font = (TTF_Font *)ttf_font;
    if (!ren || !font) return;

    int fh = TTF_FontHeight(font);
    int pad = 4;

    /* --- Event log at the bottom --- */
    int log_y = screen_h - (UI_LOG_LINES * (fh + 1)) - pad;

    /* Semi-transparent background for log */
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
    SDL_Rect log_bg = { 0, log_y - pad, screen_w,
                        UI_LOG_LINES * (fh + 1) + pad * 2 };
    SDL_RenderFillRect(ren, &log_bg);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);

    int n = ui->log.count;
    for (int i = 0; i < n; i++) {
        int line_idx = (ui->log.head - n + i + UI_LOG_LINES * 100)
                       % UI_LOG_LINES;
        int y = log_y + i * (fh + 1);
        render_text(ren, font, ui->log.lines[line_idx], pad, y,
                    ui->log.colors[line_idx]);
    }

    /* --- FPS counter (top-right) --- */
    char fps_buf[32];
    snprintf(fps_buf, sizeof(fps_buf), "FPS: %d", ui->fps);
    render_text(ren, font, fps_buf,
                screen_w - 80, pad, COL_GRAY);

    /* --- Help hint (top-left) --- */
    render_text(ren, font, "Arrows/WASD=move  Q=quit  ?=help",
                pad, pad, COL_DARKGRAY);
}
