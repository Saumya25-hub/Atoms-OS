/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Input & Events Adapter (Chromium ui::Event)
 */

#ifndef ATOMS_APAL_INPUT_H
#define ATOMS_APAL_INPUT_H

#include "../include/apal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APAL_EVENT_NONE = 0,
    APAL_EVENT_MOUSE_MOVE,
    APAL_EVENT_MOUSE_DOWN,
    APAL_EVENT_MOUSE_UP,
    APAL_EVENT_KEY_DOWN,
    APAL_EVENT_KEY_UP,
    APAL_EVENT_CHAR,
    APAL_EVENT_WINDOW_CLOSE
} apal_event_type_t;

typedef struct {
    apal_event_type_t type;
    uint32_t window_id;
    int32_t mouse_x;
    int32_t mouse_y;
    uint32_t mouse_button; /* 1=Left, 2=Right, 4=Middle */
    uint32_t key_code;
    uint32_t char_code;
    uint32_t modifiers;    /* 1=Shift, 2=Ctrl, 4=Alt */
} apal_input_event_t;

apal_status_t apal_input_poll_event(uint32_t window_id, apal_input_event_t *out_event);
apal_status_t apal_input_translate_key(uint32_t hardware_keycode, uint32_t *out_dom_code);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_INPUT_H */
