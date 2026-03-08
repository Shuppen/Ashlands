/*
 * input.c — Input handling implementation
 * C11, MIT License
 */
#include "input.h"

#include <string.h>

void input_init(InputState *inp) {
    memset(inp, 0, sizeof(InputState));
}

void input_begin_frame(InputState *inp) {
    memset(inp->keys_pressed,  0, sizeof(inp->keys_pressed));
    memset(inp->keys_released, 0, sizeof(inp->keys_released));
    inp->quit_requested = false;
    inp->window_resized = false;
}

bool input_handle_event(InputState *inp, const SDL_Event *ev) {
    switch (ev->type) {
    case SDL_QUIT:
        inp->quit_requested = true;
        return true;

    case SDL_KEYDOWN:
        if (ev->key.keysym.scancode < SDL_NUM_SCANCODES) {
            SDL_Scancode scancode = ev->key.keysym.scancode;
            if (!inp->keys_down[scancode]) {
                inp->keys_pressed[scancode] = true;
            }
            inp->keys_down[scancode] = true;
        }
        return true;

    case SDL_KEYUP:
        if (ev->key.keysym.scancode < SDL_NUM_SCANCODES) {
            SDL_Scancode scancode = ev->key.keysym.scancode;
            inp->keys_down[scancode] = false;
            inp->keys_released[scancode] = true;
        }
        return true;

    case SDL_MOUSEMOTION:
        inp->mouse_x = ev->motion.x;
        inp->mouse_y = ev->motion.y;
        return true;

    case SDL_MOUSEBUTTONDOWN:
        if (ev->button.button <= 5)
            inp->mouse_buttons[ev->button.button - 1] = true;
        return true;

    case SDL_MOUSEBUTTONUP:
        if (ev->button.button <= 5)
            inp->mouse_buttons[ev->button.button - 1] = false;
        return true;

    case SDL_WINDOWEVENT:
        if (ev->window.event == SDL_WINDOWEVENT_RESIZED) {
            inp->window_resized = true;
            inp->new_width      = ev->window.data1;
            inp->new_height     = ev->window.data2;
        }
        return true;

    default:
        return false;
    }
}

/* =========================================================
 * Key binding table
 * ========================================================= */
typedef struct { SDL_Scancode key; GameAction action; } KeyBinding;

static const KeyBinding KEY_BINDINGS[] = {
    /* Movement — arrows */
    { SDL_SCANCODE_UP,    ACTION_MOVE_N  },
    { SDL_SCANCODE_DOWN,  ACTION_MOVE_S  },
    { SDL_SCANCODE_RIGHT, ACTION_MOVE_E  },
    { SDL_SCANCODE_LEFT,  ACTION_MOVE_W  },

    /* Movement — WASD */
    { SDL_SCANCODE_W, ACTION_MOVE_N },
    { SDL_SCANCODE_S, ACTION_MOVE_S },
    { SDL_SCANCODE_D, ACTION_MOVE_E },
    { SDL_SCANCODE_A, ACTION_MOVE_W },

    /* Movement — numpad */
    { SDL_SCANCODE_KP_8, ACTION_MOVE_N  },
    { SDL_SCANCODE_KP_2, ACTION_MOVE_S  },
    { SDL_SCANCODE_KP_6, ACTION_MOVE_E  },
    { SDL_SCANCODE_KP_4, ACTION_MOVE_W  },
    { SDL_SCANCODE_KP_9, ACTION_MOVE_NE },
    { SDL_SCANCODE_KP_7, ACTION_MOVE_NW },
    { SDL_SCANCODE_KP_3, ACTION_MOVE_SE },
    { SDL_SCANCODE_KP_1, ACTION_MOVE_SW },
    { SDL_SCANCODE_KP_5, ACTION_WAIT    },

    /* Diagonal via home/end/pgup/pgdn */
    { SDL_SCANCODE_HOME,     ACTION_MOVE_NW },
    { SDL_SCANCODE_PAGEUP,   ACTION_MOVE_NE },
    { SDL_SCANCODE_END,      ACTION_MOVE_SW },
    { SDL_SCANCODE_PAGEDOWN, ACTION_MOVE_SE },

    /* Wait */
    { SDL_SCANCODE_PERIOD, ACTION_WAIT },
    { SDL_SCANCODE_SPACE,  ACTION_WAIT },

    /* Interaction */
    { SDL_SCANCODE_E, ACTION_INTERACT       },
    { SDL_SCANCODE_G, ACTION_PICKUP         },
    { SDL_SCANCODE_R, ACTION_DROP           },

    /* UI */
    { SDL_SCANCODE_I,      ACTION_OPEN_INVENTORY },
    { SDL_SCANCODE_SLASH,  ACTION_OPEN_HELP      },
    { SDL_SCANCODE_ESCAPE, ACTION_CLOSE_MENU     },
    { SDL_SCANCODE_RETURN, ACTION_CONFIRM        },
    { SDL_SCANCODE_1,      ACTION_DIALOG_OPTION_1 },
    { SDL_SCANCODE_2,      ACTION_DIALOG_OPTION_2 },
    { SDL_SCANCODE_3,      ACTION_DIALOG_OPTION_3 },
    { SDL_SCANCODE_4,      ACTION_DIALOG_OPTION_4 },
    { SDL_SCANCODE_5,      ACTION_DIALOG_OPTION_5 },
    { SDL_SCANCODE_6,      ACTION_DIALOG_OPTION_6 },
    { SDL_SCANCODE_7,      ACTION_DIALOG_OPTION_7 },
    { SDL_SCANCODE_8,      ACTION_DIALOG_OPTION_8 },

    /* Meta */
    { SDL_SCANCODE_Q, ACTION_QUIT             },
    { SDL_SCANCODE_F11, ACTION_TOGGLE_FULLSCREEN },
    { SDL_SCANCODE_F12, ACTION_SCREENSHOT       },
    { SDL_SCANCODE_F5, ACTION_SAVE_GAME },
    { SDL_SCANCODE_F9, ACTION_LOAD_GAME },
};

GameAction input_get_action(const InputState *inp) {
    size_t binding_count = sizeof(KEY_BINDINGS) / sizeof(KEY_BINDINGS[0]);
    for (size_t i = 0; i < binding_count; i++) {
        if (inp->keys_pressed[KEY_BINDINGS[i].key])
            return KEY_BINDINGS[i].action;
    }
    return ACTION_NONE;
}

bool action_to_dir(GameAction a, int *dx, int *dy) {
    *dx = 0; *dy = 0;
    switch (a) {
    case ACTION_MOVE_N:  *dy = -1; return true;
    case ACTION_MOVE_S:  *dy =  1; return true;
    case ACTION_MOVE_E:  *dx =  1; return true;
    case ACTION_MOVE_W:  *dx = -1; return true;
    case ACTION_MOVE_NE: *dx =  1; *dy = -1; return true;
    case ACTION_MOVE_NW: *dx = -1; *dy = -1; return true;
    case ACTION_MOVE_SE: *dx =  1; *dy =  1; return true;
    case ACTION_MOVE_SW: *dx = -1; *dy =  1; return true;
    default: return false;
    }
}
