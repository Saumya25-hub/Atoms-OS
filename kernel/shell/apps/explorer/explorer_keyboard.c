#include "explorer_keyboard.h"

// Standard ASCII / Keycode constants
#define KEY_ENTER     0x0D
#define KEY_BACKSPACE 0x08
#define KEY_DELETE    0x7F
#define KEY_ESC       0x1B
#define KEY_F2        0x82
#define KEY_UP        0x80
#define KEY_DOWN      0x81
#define KEY_LEFT      0x83
#define KEY_RIGHT     0x84
#define KEY_PAGEUP    0x85
#define KEY_PAGEDOWN  0x86
#define KEY_HOME      0x87
#define KEY_END       0x88

ExplorerKeyAction explorer_keyboard_dispatch(uint32_t keycode, bool ctrl_pressed, bool shift_pressed) {
    (void)shift_pressed;
    
    if (ctrl_pressed) {
        switch (keycode) {
            case 'a': case 'A': return KEY_ACTION_SELECT_ALL;
            case 'c': case 'C': return KEY_ACTION_COPY;
            case 'x': case 'X': return KEY_ACTION_CUT;
            case 'v': case 'V': return KEY_ACTION_PASTE;
            default: break;
        }
    }
    
    switch (keycode) {
        case KEY_ENTER:     return KEY_ACTION_NAV_OPEN;
        case KEY_BACKSPACE: return KEY_ACTION_NAV_UP;
        case KEY_DELETE:    return KEY_ACTION_DELETE;
        case KEY_F2:        return KEY_ACTION_RENAME;
        case KEY_ESC:       return KEY_ACTION_CANCEL;
        case KEY_UP:        return KEY_ACTION_MOVE_UP;
        case KEY_DOWN:      return KEY_ACTION_MOVE_DOWN;
        case KEY_LEFT:      return KEY_ACTION_MOVE_LEFT;
        case KEY_RIGHT:     return KEY_ACTION_MOVE_RIGHT;
        case KEY_PAGEUP:    return KEY_ACTION_PAGE_UP;
        case KEY_PAGEDOWN:  return KEY_ACTION_PAGE_DOWN;
        case KEY_HOME:      return KEY_ACTION_HOME;
        case KEY_END:       return KEY_ACTION_END;
        default:            return KEY_ACTION_NONE;
    }
}
