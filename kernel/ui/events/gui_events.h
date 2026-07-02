#ifndef BOS_GUI_EVENTS_H
#define BOS_GUI_EVENTS_H

#include <stdint.h>

typedef enum {
    BOS_GUI_EVENT_NONE = 0,
    BOS_GUI_EVENT_CLICK = 1,
    BOS_GUI_EVENT_CLOSE = 2
} BOS_GUIEventType;

typedef struct {
    BOS_GUIEventType type;
    uint32_t control_id;
    uint32_t window_id;
    uint64_t user_callback;
} BOS_GUIEvent;

#define MAX_GUI_EVENTS_PER_QUEUE 64

typedef struct {
    uint32_t pid;
    BOS_GUIEvent events[MAX_GUI_EVENTS_PER_QUEUE];
    uint32_t head;
    uint32_t tail;
} BOS_GUIEventQueue;

void bos_gui_events_init(void);
void bos_gui_event_push(uint32_t pid, BOS_GUIEventType type, uint32_t control_id, uint32_t window_id, uint64_t user_callback);
int bos_gui_event_pop(uint32_t pid, BOS_GUIEvent* out_event);
void bos_gui_event_cleanup_pid(uint32_t pid);

#endif
