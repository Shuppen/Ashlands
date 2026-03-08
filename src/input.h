/*
 * input.h — Keyboard, mouse, gamepad input
 * C11, MIT License
 */
#ifndef ASHLANDS_INPUT_H
#define ASHLANDS_INPUT_H

#include <stdbool.h>
#include <SDL2/SDL.h>

/* =========================================================
 * Action abstraction — maps physical keys → game actions
 * ========================================================= */
typedef enum {
    ACTION_NONE = 0,

    /* Movement */
    ACTION_MOVE_N,
    ACTION_MOVE_S,
    ACTION_MOVE_E,
    ACTION_MOVE_W,
    ACTION_MOVE_NE,
    ACTION_MOVE_NW,
    ACTION_MOVE_SE,
    ACTION_MOVE_SW,
    ACTION_WAIT,

    /* Interaction */
    ACTION_INTERACT,
    ACTION_PICKUP,
    ACTION_DROP,

    /* UI */
    ACTION_OPEN_INVENTORY,
    ACTION_OPEN_HELP,
    ACTION_CLOSE_MENU,
    ACTION_CONFIRM,
    ACTION_CANCEL,
    ACTION_DIALOG_OPTION_1,
    ACTION_DIALOG_OPTION_2,
    ACTION_DIALOG_OPTION_3,
    ACTION_DIALOG_OPTION_4,
    ACTION_DIALOG_OPTION_5,
    ACTION_DIALOG_OPTION_6,
    ACTION_DIALOG_OPTION_7,
    ACTION_DIALOG_OPTION_8,

    /* Meta */
    ACTION_QUIT,
    ACTION_TOGGLE_FULLSCREEN,
    ACTION_SCREENSHOT,
    ACTION_SAVE_GAME,
    ACTION_LOAD_GAME,

    ACTION_COUNT
} GameAction;

/* =========================================================
 * InputState
 * ========================================================= */
typedef struct {
    bool keys_down[SDL_NUM_SCANCODES];
    bool keys_pressed[SDL_NUM_SCANCODES];  /* true only on the frame pressed */
    bool keys_released[SDL_NUM_SCANCODES];

    int  mouse_x, mouse_y;
    bool mouse_buttons[5];

    bool quit_requested;
    bool window_resized;
    int  new_width, new_height;
} InputState;

/* =========================================================
 * API
 * ========================================================= */
void input_init(InputState *inp);

/* Call at start of each frame — clears per-frame flags */
void input_begin_frame(InputState *inp);

/* Process one SDL_Event. Returns false if the event was not consumed. */
bool input_handle_event(InputState *inp, const SDL_Event *ev);

/* Translate current state → game action (first matching wins) */
GameAction input_get_action(const InputState *inp);

/* Direction from an action (fills dx/dy; returns false if not movement) */
bool action_to_dir(GameAction a, int *dx, int *dy);

#endif /* ASHLANDS_INPUT_H */
