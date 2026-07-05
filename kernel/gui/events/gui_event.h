#ifndef GUI_EVENT_H
#define GUI_EVENT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    GUI_EVENT_NONE = 0,
    GUI_EVENT_MOUSE_MOVE,
    GUI_EVENT_MOUSE_DOWN,
    GUI_EVENT_MOUSE_UP,
    GUI_EVENT_MOUSE_ENTER,
    GUI_EVENT_MOUSE_LEAVE,
    GUI_EVENT_DOUBLE_CLICK,
    GUI_EVENT_KEY_DOWN,
    GUI_EVENT_KEY_UP,
    GUI_EVENT_PAINT,
    GUI_EVENT_WINDOW_CLOSE,
    GUI_EVENT_WINDOW_RESIZE,
    GUI_EVENT_STARTMENU_OPEN
} GUIEventType;

typedef struct {
    GUIEventType type;
    uint64_t target_surface_id;
    
    // Payload (union for memory efficiency)
    union {
        struct {
            int x;
            int y;
            uint8_t buttons;
        } mouse;
        struct {
            uint32_t key_code;
            uint32_t modifiers;
        } key;
        struct {
            int width;
            int height;
        } resize;
    } data;
} GUIEvent;

#define MAX_EVENTS_PER_QUEUE 64

typedef struct {
    GUIEvent events[MAX_EVENTS_PER_QUEUE];
    int head;
    int tail;
    int count;
} GUIEventQueue;

void gui_event_queue_init(GUIEventQueue* queue);
bool gui_event_push(GUIEventQueue* queue, const GUIEvent* event);
bool gui_event_pop(GUIEventQueue* queue, GUIEvent* out_event);

#endif // GUI_EVENT_H
