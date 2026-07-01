#ifndef BWE_EVENTS_H
#define BWE_EVENTS_H

#include <stdint.h>
#include <stdbool.h>

// BWE Event Type Enum
typedef enum {
    BWE_EVENT_NONE,
    BWE_EVENT_MOUSE_MOVE,
    BWE_EVENT_MOUSE_DOWN,
    BWE_EVENT_MOUSE_UP,
    BWE_EVENT_MOUSE_DBLCLICK,
    BWE_EVENT_MOUSE_WHEEL,
    BWE_EVENT_KEY_DOWN,
    BWE_EVENT_KEY_UP,
    BWE_EVENT_FOCUS_GAIN,
    BWE_EVENT_FOCUS_LOSS,
    BWE_EVENT_WINDOW_RESIZE,
    BWE_EVENT_WINDOW_MOVE,
    BWE_EVENT_WINDOW_CLOSE,
    BWE_EVENT_PAINT,
    BWE_EVENT_TIMER,
    BWE_EVENT_USER,
    BWE_EVENT_KERNEL
} BWE_EventType;

// Event Propagation Status Flags
#define BWE_EVENT_IGNORED  0
#define BWE_EVENT_HANDLED  1

// Keyboard Event Structure
typedef struct {
    uint32_t key_code;
    uint32_t modifiers;
} BWE_KeyEvent;

// Mouse Event Structure
typedef struct {
    int32_t  x;
    int32_t  y;
    uint8_t  buttons;
    int8_t   wheel_delta;
} BWE_MouseEvent;

// Master BWE Event Structure
typedef struct BWE_Event {
    BWE_EventType type;
    uint32_t      target_id;
    union {
        BWE_MouseEvent mouse;
        BWE_KeyEvent   key;
        struct {
            uint32_t param1;
            uint32_t param2;
        } system;
    } data;
} BWE_Event;

#endif // BWE_EVENTS_H
