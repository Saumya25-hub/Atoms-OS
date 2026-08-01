#ifndef BSEC_EXPLORER_KEYBOARD_H
#define BSEC_EXPLORER_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    KEY_ACTION_NONE = 0,
    KEY_ACTION_NAV_OPEN,
    KEY_ACTION_NAV_BACK,
    KEY_ACTION_NAV_UP,
    KEY_ACTION_RENAME,
    KEY_ACTION_DELETE,
    KEY_ACTION_COPY,
    KEY_ACTION_CUT,
    KEY_ACTION_PASTE,
    KEY_ACTION_SELECT_ALL,
    KEY_ACTION_CANCEL,
    KEY_ACTION_MOVE_UP,
    KEY_ACTION_MOVE_DOWN,
    KEY_ACTION_MOVE_LEFT,
    KEY_ACTION_MOVE_RIGHT,
    KEY_ACTION_PAGE_UP,
    KEY_ACTION_PAGE_DOWN,
    KEY_ACTION_HOME,
    KEY_ACTION_END
} ExplorerKeyAction;

ExplorerKeyAction explorer_keyboard_dispatch(uint32_t keycode, bool ctrl_pressed, bool shift_pressed);

#endif // BSEC_EXPLORER_KEYBOARD_H
