#include "dispatcher_queue.h"
#include "dispatcher_diag.h"
#include "kernel/core/lib/include/string.h"

static DispatcherEvent g_queue[DISPATCHER_QUEUE_SIZE];
static volatile uint32_t g_head = 0;
static volatile uint32_t g_tail = 0;

void dispatcher_queue_init(void) {
    g_head = 0;
    g_tail = 0;
    memset(g_queue, 0, sizeof(g_queue));
}

uint32_t dispatcher_queue_get_depth(void) {
    uint32_t head = g_head;
    uint32_t tail = g_tail;
    if (tail >= head) {
        return tail - head;
    }
    return DISPATCHER_QUEUE_SIZE - (head - tail);
}

bool dispatcher_queue_is_empty(void) {
    return g_head == g_tail;
}

bool dispatcher_queue_is_full(void) {
    return ((g_tail + 1) % DISPATCHER_QUEUE_SIZE) == g_head;
}

bool dispatcher_queue_push(const DispatcherEvent* event) {
    if (!event) return false;

    dispatcher_diag_record_received();

    uint32_t next_tail = (g_tail + 1) % DISPATCHER_QUEUE_SIZE;
    if (next_tail == g_head) {
        // Queue overflow: drop event and log diagnostic
        dispatcher_diag_record_overflow();
        return false;
    }

    g_queue[g_tail] = *event;
    g_tail = next_tail;

    dispatcher_diag_update_depth(dispatcher_queue_get_depth());
    return true;
}

bool dispatcher_queue_pop(DispatcherEvent* out_event) {
    if (!out_event || g_head == g_tail) {
        return false;
    }

    *out_event = g_queue[g_head];
    g_head = (g_head + 1) % DISPATCHER_QUEUE_SIZE;

    dispatcher_diag_update_depth(dispatcher_queue_get_depth());
    return true;
}

void dispatcher_queue_clear(void) {
    g_head = 0;
    g_tail = 0;
    dispatcher_diag_update_depth(0);
}
