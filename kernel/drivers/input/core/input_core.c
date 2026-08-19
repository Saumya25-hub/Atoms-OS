#include "input_core.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/display/display.h"
#include "kernel/drivers/input/dispatcher/dispatcher.h"

#define INPUT_CORE_QUEUE_SIZE 1024
#define INPUT_CORE_MAX_CONSUMERS 16

// Central Lockless Ring Buffer
static InputCoreEvent g_core_queue[INPUT_CORE_QUEUE_SIZE];
static volatile uint32_t g_core_head = 0;
static volatile uint32_t g_core_tail = 0;

// Diagnostics
volatile uint64_t g_input_core_events_count = 0;
static uint32_t g_total_pushed = 0;
static uint32_t g_total_dispatched = 0;
static uint32_t g_total_dropped = 0;

// Consumer Registry
typedef struct {
    char name[32];
    InputConsumerPriority priority;
    InputConsumerCallback callback;
    void* user_data;
    bool active;
} InputConsumerEntry;

static InputConsumerEntry g_consumers[INPUT_CORE_MAX_CONSUMERS];
static uint32_t g_consumer_count = 0;

void input_core_init(void) {
    g_core_head = 0;
    g_core_tail = 0;
    g_total_pushed = 0;
    g_total_dispatched = 0;
    g_total_dropped = 0;
    g_consumer_count = 0;
    
    for (int i = 0; i < INPUT_CORE_MAX_CONSUMERS; i++) {
        g_consumers[i].active = false;
        g_consumers[i].callback = 0;
    }
    
    display_print("[INPUT CORE] Subsystem Initialized successfully (Queue Size: 1024)\n");
}

bool input_core_push_event(const InputCoreEvent* event) {
    if (!event) return false;
    
    uint32_t next_head = (g_core_head + 1) % INPUT_CORE_QUEUE_SIZE;
    if (next_head == g_core_tail) {
        g_total_dropped++;
        return false; // Queue full
    }
    
    InputCoreEvent copy = *event;
    if (copy.timestamp_us == 0) {
        copy.timestamp_us = timer_get_ticks() * 1000;
    }
    
    g_core_queue[g_core_head] = copy;
    g_core_head = next_head;
    g_total_pushed++;
    g_input_core_events_count++;
    return true;
}

bool input_core_pop_event(InputCoreEvent* out_event) {
    if (!out_event || g_core_tail == g_core_head) {
        return false; // Queue empty
    }
    
    *out_event = g_core_queue[g_core_tail];
    g_core_tail = (g_core_tail + 1) % INPUT_CORE_QUEUE_SIZE;
    return true;
}

static void sort_consumers_by_priority(void) {
    for (uint32_t i = 0; i < g_consumer_count; i++) {
        for (uint32_t j = i + 1; j < g_consumer_count; j++) {
            if (g_consumers[j].priority < g_consumers[i].priority) {
                InputConsumerEntry temp = g_consumers[i];
                g_consumers[i] = g_consumers[j];
                g_consumers[j] = temp;
            }
        }
    }
}

bool input_core_register_consumer(const char* name, InputConsumerPriority priority, InputConsumerCallback callback, void* user_data) {
    if (!callback || g_consumer_count >= INPUT_CORE_MAX_CONSUMERS) {
        return false;
    }
    
    InputConsumerEntry* entry = &g_consumers[g_consumer_count++];
    int i = 0;
    while (name && name[i] && i < 31) {
        entry->name[i] = name[i];
        i++;
    }
    entry->name[i] = '\0';
    entry->priority = priority;
    entry->callback = callback;
    entry->user_data = user_data;
    entry->active = true;
    
    sort_consumers_by_priority();
    display_print("[INPUT CORE] Registered Consumer: ");
    display_print(entry->name);
    display_print("\n");
    return true;
}

void input_core_dispatch_events(void) {
    InputCoreEvent ev;
    while (input_core_pop_event(&ev)) {
        g_total_dispatched++;
        for (uint32_t i = 0; i < g_consumer_count; i++) {
            if (g_consumers[i].active && g_consumers[i].callback) {
                bool consumed = g_consumers[i].callback(&ev, g_consumers[i].user_data);
                if (consumed) {
                    break;
                }
            }
        }

        // Push non-pointer events directly to Event Dispatcher
        // (Pointer events are pushed by Pointer Engine after sub-pixel processing)
        if (ev.type != INPUT_EVENT_TYPE_MOTION_RELATIVE &&
            ev.type != INPUT_EVENT_TYPE_MOTION_ABSOLUTE &&
            ev.type != INPUT_EVENT_TYPE_BUTTON) {
            dispatcher_push_event(&ev);
        }
    }
}

void input_core_get_diagnostics(uint32_t* total_pushed, uint32_t* total_dispatched, uint32_t* total_dropped, uint32_t* queue_size) {
    if (total_pushed) *total_pushed = g_total_pushed;
    if (total_dispatched) *total_dispatched = g_total_dispatched;
    if (total_dropped) *total_dropped = g_total_dropped;
    if (queue_size) {
        uint32_t h = g_core_head;
        uint32_t t = g_core_tail;
        *queue_size = (h >= t) ? (h - t) : (INPUT_CORE_QUEUE_SIZE - t + h);
    }
}
