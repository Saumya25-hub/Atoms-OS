/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Input & Events Implementation
 */

#include "apal_input.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"
#include <string.h>

#define SYS_GUI_POLL_EVENT 22ULL

typedef struct {
    uint32_t        abi_version;
    uint32_t        type;
    uint32_t        window_id;
    int32_t         mouse_x;
    int32_t         mouse_y;
    uint32_t        mouse_btn;
    uint32_t        key_code;
    uint32_t        ascii_char;
    uint32_t        modifiers;
    uint32_t        reserved;
} apal_raw_bos_event_t;

apal_status_t apal_input_poll_event(uint32_t window_id, apal_input_event_t *out_event) {
    if (!out_event) return APAL_ERR_INVALID_PARAM;
    memset(out_event, 0, sizeof(apal_input_event_t));

    apal_raw_bos_event_t raw;
    memset(&raw, 0, sizeof(raw));
    raw.abi_version = 1;

    int64_t ret = __syscall3(SYS_GUI_POLL_EVENT, (int64_t)window_id, (int64_t)&raw, sizeof(raw));
    if (ret != 0 || raw.type == 0) {
        out_event->type = APAL_EVENT_NONE;
        return APAL_OK;
    }

    out_event->window_id = raw.window_id;
    out_event->mouse_x = raw.mouse_x;
    out_event->mouse_y = raw.mouse_y;
    out_event->mouse_button = raw.mouse_btn;
    out_event->key_code = raw.key_code;
    out_event->char_code = raw.ascii_char;
    out_event->modifiers = raw.modifiers;

    switch (raw.type) {
        case 1: /* BOS_GUI_EVENT_CLICK */
        case 6: /* BOS_GUI_EVENT_MOUSE_DOWN */
            out_event->type = APAL_EVENT_MOUSE_DOWN;
            break;
        case 7: /* BOS_GUI_EVENT_MOUSE_UP */
            out_event->type = APAL_EVENT_MOUSE_UP;
            break;
        case 5: /* BOS_GUI_EVENT_MOUSE_MOVE */
            out_event->type = APAL_EVENT_MOUSE_MOVE;
            break;
        case 3: /* BOS_GUI_EVENT_KEY_DOWN */
            out_event->type = APAL_EVENT_KEY_DOWN;
            break;
        case 4: /* BOS_GUI_EVENT_KEY_UP */
            out_event->type = APAL_EVENT_KEY_UP;
            break;
        case 2: /* BOS_GUI_EVENT_CLOSE */
            out_event->type = APAL_EVENT_WINDOW_CLOSE;
            break;
        default:
            out_event->type = APAL_EVENT_NONE;
            break;
    }

    return APAL_OK;
}

apal_status_t apal_input_translate_key(uint32_t hardware_keycode, uint32_t *out_dom_code) {
    if (!out_dom_code) return APAL_ERR_INVALID_PARAM;
    /* Map PS/2 or USB HID keycodes to DOM / Chromium standard virtual keys */
    if (hardware_keycode >= 0x04 && hardware_keycode <= 0x1D) {
        /* USB HID Key A (0x04) -> 'A' (65) */
        *out_dom_code = 'A' + (hardware_keycode - 0x04);
    } else if (hardware_keycode >= 0x1E && hardware_keycode <= 0x27) {
        /* USB HID 1-9, 0 */
        *out_dom_code = (hardware_keycode == 0x27) ? '0' : ('1' + (hardware_keycode - 0x1E));
    } else {
        *out_dom_code = hardware_keycode;
    }
    return APAL_OK;
}
