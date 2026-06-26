#ifndef BOVISUAL_EVENTS_H
#define BOVISUAL_EVENTS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BV_EVENT_MOUSE_MOVE,
    BV_EVENT_MOUSE_DOWN,
    BV_EVENT_MOUSE_UP,
    BV_EVENT_KEY_DOWN,
    BV_EVENT_KEY_UP
} BVEventType;

typedef struct {
    BVEventType type;
    int32_t mouse_x;
    int32_t mouse_y;
    uint8_t mouse_buttons; // bit 0 = Left, bit 1 = Right, bit 2 = Middle
    uint8_t key_code;
} BVEvent;

#endif // BOVISUAL_EVENTS_H
