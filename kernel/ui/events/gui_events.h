#ifndef BOS_GUI_EVENTS_H
#define BOS_GUI_EVENTS_H

#include <stdint.h>

typedef enum {
    BOS_GUI_EVENT_NONE = 0,
    BOS_GUI_EVENT_CLICK = 1,
    BOS_GUI_EVENT_CLOSE = 2,
    BOS_GUI_EVENT_KEY_DOWN = 3,
    BOS_GUI_EVENT_KEY_UP = 4,
    BOS_GUI_EVENT_MOUSE_MOVE = 5,
    BOS_GUI_EVENT_MOUSE_DOWN = 6,
    BOS_GUI_EVENT_MOUSE_UP = 7
} BOS_GUIEventType;

typedef struct {
    BOS_GUIEventType type;
    uint32_t control_id;
    uint32_t window_id;
    union {
        uint64_t user_callback;
        struct {
            uint32_t keycode;
            uint32_t modifiers;
            uint32_t ascii;
        } key;
        struct {
            int32_t x;
            int32_t y;
            uint32_t buttons;
        } mouse;
    };
} BOS_GUIEvent;

#define MAX_GUI_EVENTS_PER_QUEUE 64

typedef struct {
    uint32_t pid;
    BOS_GUIEvent events[MAX_GUI_EVENTS_PER_QUEUE];
    uint32_t head;
    uint32_t tail;
} BOS_GUIEventQueue;

void bos_gui_events_init(void);
void bos_gui_event_push_raw(uint32_t pid, const BOS_GUIEvent* ev);
void bos_gui_event_push(uint32_t pid, BOS_GUIEventType type, uint32_t control_id, uint32_t window_id, uint64_t user_callback);
int bos_gui_event_pop(uint32_t pid, BOS_GUIEvent* out_event);
void bos_gui_event_cleanup_pid(uint32_t pid);

#endif
