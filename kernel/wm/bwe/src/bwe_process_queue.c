#include "../include/bwe_process_queue.h"
#include "kernel/core/lib/include/string.h"

#define MAX_PIDS 1024

typedef struct {
    BOS_InputEvent events[BOS_INPUT_MAX_QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} ProcessInputQueue;

static ProcessInputQueue g_process_queues[MAX_PIDS];
volatile uint64_t g_bwe_process_events_pushed = 0;
volatile uint64_t g_bwe_process_events_popped = 0;
volatile uint64_t g_bwe_process_key_events_pushed = 0;
volatile uint64_t g_bwe_process_key_events_popped = 0;
volatile uint32_t g_bwe_process_last_push_pid = 0;
volatile uint32_t g_bwe_process_last_pop_pid = 0;

extern void display_print(const char*);
extern void display_print_dec(uint32_t);

void bwe_process_queue_init(void) {
    for (int i = 0; i < MAX_PIDS; i++) {
        g_process_queues[i].head = 0;
        g_process_queues[i].tail = 0;
        g_process_queues[i].count = 0;
    }
}

void bwe_process_queue_push(uint32_t owner_pid, const BOS_InputEvent* event) {
    if (owner_pid >= MAX_PIDS) return;
    
    ProcessInputQueue* q = &g_process_queues[owner_pid];
    bool is_key = event->type == BOS_INPUT_KEY_DOWN || event->type == BOS_INPUT_KEY_UP;
    
    __asm__ volatile("cli");
    
    // Mouse move coalescing
    if (event->type == BOS_INPUT_MOUSE_MOVE && q->count > 0) {
        uint32_t last_idx = (q->tail == 0) ? (BOS_INPUT_MAX_QUEUE_SIZE - 1) : (q->tail - 1);
        BOS_InputEvent* last_ev = &q->events[last_idx];
        
        if (last_ev->type == BOS_INPUT_MOUSE_MOVE) {
            // Coalesce: update position and accumulate delta
            last_ev->data.mouse.delta_x += event->data.mouse.delta_x;
            last_ev->data.mouse.delta_y += event->data.mouse.delta_y;
            last_ev->data.mouse.screen_x = event->data.mouse.screen_x;
            last_ev->data.mouse.screen_y = event->data.mouse.screen_y;
            last_ev->data.mouse.local_x = event->data.mouse.local_x;
            last_ev->data.mouse.local_y = event->data.mouse.local_y;
            last_ev->timestamp = event->timestamp;
            g_bwe_process_events_pushed++;
            __asm__ volatile("sti");
            return;
        }
    }
    
    if (q->count >= BOS_INPUT_MAX_QUEUE_SIZE) {
        // Queue full, drop event (or we could drop oldest, but requirement says 
        // semantic events must not be silently displaced by mouse movement.
        // We drop newest if full).
        __asm__ volatile("sti");
        return;
    }
    
    q->events[q->tail] = *event;
    q->tail = (q->tail + 1) % BOS_INPUT_MAX_QUEUE_SIZE;
    q->count++;
    g_bwe_process_events_pushed++;
    g_bwe_process_last_push_pid = owner_pid;
    if (is_key) {
        g_bwe_process_key_events_pushed++;
    }
    
    __asm__ volatile("sti");
}

bool bwe_process_queue_pop(uint32_t owner_pid, BOS_InputEvent* out_event) {
    if (owner_pid >= MAX_PIDS) return false;
    
    ProcessInputQueue* q = &g_process_queues[owner_pid];
    
    __asm__ volatile("cli");
    
    if (q->count == 0) {
        __asm__ volatile("sti");
        return false;
    }
    
    *out_event = q->events[q->head];
    q->head = (q->head + 1) % BOS_INPUT_MAX_QUEUE_SIZE;
    q->count--;
    g_bwe_process_events_popped++;
    g_bwe_process_last_pop_pid = owner_pid;
    if (out_event->type == BOS_INPUT_KEY_DOWN || out_event->type == BOS_INPUT_KEY_UP) {
        g_bwe_process_key_events_popped++;
    }
    
    __asm__ volatile("sti");
    return true;
}
