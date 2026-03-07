/*
 * ui.h — In-game user interface
 * C11, MIT License
 */
#ifndef ASHLANDS_UI_H
#define ASHLANDS_UI_H

#include <stdint.h>
#include <stdbool.h>
#include "../include/ashlands.h"

#define UI_LOG_LINES   8
#define UI_LOG_MAX_LEN 128

typedef struct {
    char    lines[UI_LOG_LINES][UI_LOG_MAX_LEN];
    uint32_t colors[UI_LOG_LINES];
    int     count;
    int     head;   /* circular buffer head */
} UILog;

typedef struct UIState {
    UILog   log;
    bool    show_inventory;
    bool    show_stats;
    bool    show_help;
    int     player_id;
    int     fps;
} UIState;

void  ui_state_init(UIState *ui, int player_id);

/* Add a message to the event log */
void  ui_log(UIState *ui, const char *text, uint32_t color);

/* Render the entire UI overlay (log, stats bar, etc.) */
void  ui_render(const UIState *ui, void *sdl_renderer, void *ttf_font,
                int screen_w, int screen_h);

#endif /* ASHLANDS_UI_H */
