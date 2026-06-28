#include "gui_events.h"

static BOS_GUIEventQueue g_queues[64];

void bos_gui_events_init(void) {
    for (int i=0; i<64; i++) {
        g_queues[i].pid = 0;
        g_queues[i].head = 0;
        g_queues[i].tail = 0;
    }
}

static BOS_GUIEventQueue* get_or_create_queue(uint32_t pid) {
    if (pid == 0) return 0;
    for (int i=0; i<64; i++) {
        if (g_queues[i].pid == pid) return &g_queues[i];
    }
    for (int i=0; i<64; i++) {
        if (g_queues[i].pid == 0) {
            g_queues[i].pid = pid;
            g_queues[i].head = 0;
            g_queues[i].tail = 0;
            return &g_queues[i];
        }
    }
    return 0;
}

void bos_gui_event_push(uint32_t pid, BOS_GUIEventType type, uint32_t control_id, uint32_t window_id, uint64_t user_callback) {
    BOS_GUIEventQueue* q = get_or_create_queue(pid);
    if (!q) return;
    
    uint32_t next = (q->tail + 1) % MAX_GUI_EVENTS_PER_QUEUE;
    if (next == q->head) return; // Full
    
    q->events[q->tail].type = type;
    q->events[q->tail].control_id = control_id;
    q->events[q->tail].window_id = window_id;
    q->events[q->tail].user_callback = user_callback;
    
    q->tail = next;
}

int bos_gui_event_pop(uint32_t pid, BOS_GUIEvent* out_event) {
    BOS_GUIEventQueue* q = get_or_create_queue(pid);
    if (!q || q->head == q->tail) return 0; // Empty or not found
    
    *out_event = q->events[q->head];
    q->head = (q->head + 1) % MAX_GUI_EVENTS_PER_QUEUE;
    return 1;
}

void bos_gui_event_cleanup_pid(uint32_t pid) {
    for (int i=0; i<64; i++) {
        if (g_queues[i].pid == pid) {
            g_queues[i].pid = 0;
            g_queues[i].head = 0;
            g_queues[i].tail = 0;
        }
    }
}
