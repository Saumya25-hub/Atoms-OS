#include "platform/include/bos_events.h"
#include "kernel/core/lib/include/string.h"

void BOS_EventQueue_Init(BOS_EventQueue* queue) {
    if (!queue) return;
    memset(queue, 0, sizeof(BOS_EventQueue));
}

bool BOS_EventQueue_IsEmpty(const BOS_EventQueue* queue) {
    if (!queue) return true;
    return (queue->head == queue->tail);
}

bool BOS_EventQueue_IsFull(const BOS_EventQueue* queue) {
    if (!queue) return false;
    return (((queue->head + 1) % BOS_EVENT_QUEUE_CAPACITY) == queue->tail);
}

BOS_Result BOS_EventQueue_Push(BOS_EventQueue* queue, const BOS_Event* event) {
    if (!queue || !event) return BOS_ERROR_INVALID_ARGUMENT;

    /* Mouse Motion Coalescing Optimization:
       If new event is MOUSE_MOVE and the queue tail contains an unconsumed MOUSE_MOVE for the same window,
       coalesce the position in-place to prevent queue overflow and mouse stutter. */
    if (event->type == BOS_EVENT_MOUSE_MOVE && !BOS_EventQueue_IsEmpty(queue)) {
        uint32_t last_idx = (queue->head > 0) ? (queue->head - 1) : (BOS_EVENT_QUEUE_CAPACITY - 1);
        if (queue->events[last_idx].type == BOS_EVENT_MOUSE_MOVE &&
            queue->events[last_idx].target_window == event->target_window) {
            
            queue->events[last_idx].data.mouse_move.x = event->data.mouse_move.x;
            queue->events[last_idx].data.mouse_move.y = event->data.mouse_move.y;
            queue->events[last_idx].data.mouse_move.delta_x += event->data.mouse_move.delta_x;
            queue->events[last_idx].data.mouse_move.delta_y += event->data.mouse_move.delta_y;
            queue->events[last_idx].timestamp_us = event->timestamp_us;
            queue->coalesced_events++;
            return BOS_SUCCESS;
        }
    }

    /* Standard Push into Ring Buffer */
    uint32_t next_head = (queue->head + 1) % BOS_EVENT_QUEUE_CAPACITY;
    if (next_head == queue->tail) {
        queue->dropped_events++;
        return BOS_ERROR_QUEUE_FULL;
    }

    queue->events[queue->head] = *event;
    queue->head = next_head;
    return BOS_SUCCESS;
}

BOS_Result BOS_EventQueue_Pop(BOS_EventQueue* queue, BOS_Event* out_event) {
    if (!queue || !out_event) return BOS_ERROR_INVALID_ARGUMENT;

    if (BOS_EventQueue_IsEmpty(queue)) {
        return BOS_ERROR_QUEUE_EMPTY;
    }

    *out_event = queue->events[queue->tail];
    queue->tail = (queue->tail + 1) % BOS_EVENT_QUEUE_CAPACITY;
    return BOS_SUCCESS;
}

BOS_Result BOS_EventQueue_Peek(BOS_EventQueue* queue, BOS_Event* out_event) {
    if (!queue || !out_event) return BOS_ERROR_INVALID_ARGUMENT;

    if (BOS_EventQueue_IsEmpty(queue)) {
        return BOS_ERROR_QUEUE_EMPTY;
    }

    *out_event = queue->events[queue->tail];
    return BOS_SUCCESS;
}
