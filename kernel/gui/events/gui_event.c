#include "gui_event.h"
#include <stddef.h>

void gui_event_queue_init(GUIEventQueue* queue) {
    if (!queue) return;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

bool gui_event_push(GUIEventQueue* queue, const GUIEvent* event) {
    if (!queue || !event) return false;
    
    if (queue->count >= MAX_EVENTS_PER_QUEUE) {
        // Queue full, drop event (or we could drop oldest)
        return false;
    }
    
    queue->events[queue->tail] = *event;
    queue->tail = (queue->tail + 1) % MAX_EVENTS_PER_QUEUE;
    queue->count++;
    
    return true;
}

bool gui_event_pop(GUIEventQueue* queue, GUIEvent* out_event) {
    if (!queue || !out_event) return false;
    
    if (queue->count == 0) {
        return false; // Empty
    }
    
    *out_event = queue->events[queue->head];
    queue->head = (queue->head + 1) % MAX_EVENTS_PER_QUEUE;
    queue->count--;
    
    return true;
}
