#include "npc.h"
#include "ui.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

static void dialog_render_text(SDL_Renderer *ren, TTF_Font *font,
                               const char *text, int x, int y, uint32_t color) {
    SDL_Color c = { RGBA_R(color), RGBA_G(color), RGBA_B(color), RGBA_A(color) };
    SDL_Surface *surf;
    SDL_Texture *tex;
    SDL_Rect dst;

    if (!ren || !font || !text || !text[0]) {
        return;
    }

    surf = TTF_RenderUTF8_Blended_Wrapped(font, text, c, 520);
    if (!surf) {
        return;
    }

    tex = SDL_CreateTextureFromSurface(ren, surf);
    if (!tex) {
        SDL_FreeSurface(surf);
        return;
    }

    dst.x = x;
    dst.y = y;
    dst.w = surf->w;
    dst.h = surf->h;
    SDL_FreeSurface(surf);
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

void dialog_ui_render(UIState *ui, int player_id) {
    const DialogNode *node = dialog_get_current(player_id);

    if (!ui) {
        return;
    }

    ui->dialog_open = node != NULL;
}

void dialog_ui_draw(const UIState *ui, void *sdl_renderer, void *ttf_font,
                    int screen_w, int screen_h) {
    const DialogNode *node;
    DialogOption visible[MAX_DIALOG_OPTIONS];
    int visible_count;
    SDL_Renderer *ren = (SDL_Renderer *)sdl_renderer;
    TTF_Font *font = (TTF_Font *)ttf_font;
    SDL_Rect panel;

    if (!ui || !ui->dialog_open || !ren || !font) {
        return;
    }

    node = dialog_get_current(ui->player_id);
    if (!node) {
        return;
    }

    visible_count = npc_get_visible_options(ui->player_id, visible, MAX_DIALOG_OPTIONS);
    if (visible_count <= 0) {
        return;
    }

    panel.w = screen_w - 80;
    panel.h = screen_h / 3;
    panel.x = 40;
    panel.y = screen_h - panel.h - 40;

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 8, 8, 8, 220);
    SDL_RenderFillRect(ren, &panel);
    SDL_SetRenderDrawColor(ren, 170, 130, 80, 255);
    SDL_RenderDrawRect(ren, &panel);

    dialog_render_text(ren, font, node->text, panel.x + 16, panel.y + 16, COL_WHITE);
    for (int i = 0; i < visible_count; i++) {
        char buf[192];
        snprintf(buf, sizeof(buf), "%d. %s", i + 1, visible[i].text);
        dialog_render_text(ren, font, buf,
                           panel.x + 24,
                           panel.y + 90 + i * 24,
                           i == 0 ? COL_YELLOW : COL_GRAY);
    }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
}
