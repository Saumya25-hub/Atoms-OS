/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * message_queue.c — Per-Channel Message Queue Implementation
 *
 * Circular buffer per channel slot. Supports blocking and non-blocking modes.
 */

#include "kernel/ipc/message/message_queue.h"
#include "kernel/ipc/channels/channel_manager.h"
#include "kernel/ipc/permissions/ipc_permissions.h"
#include "kernel/ipc/debug/ipc_debug.h"

/* Per-channel circular message queue */
typedef struct {
    ipc_message_t  messages[IPC_MESSAGE_QUEUE_DEPTH];
    uint32_t       head;
    uint32_t       tail;
    uint32_t       count;
} ipc_mq_ring_t;

static ipc_mq_ring_t g_message_queues[IPC_MAX_CHANNELS];

void ipc_message_queue_init(void) {
    for (uint32_t i = 0; i < IPC_MAX_CHANNELS; i++) {
        g_message_queues[i].head  = 0;
        g_message_queues[i].tail  = 0;
        g_message_queues[i].count = 0;
    }
    ipc_debug_log(IPC_LOG_INFO, "MSG_QUEUE", "Message Queue Engine Initialized");
}

void ipc_mq_clear(ipc_channel_handle_t channel) {
    if (channel >= IPC_MAX_CHANNELS) return;
    g_message_queues[channel].head  = 0;
    g_message_queues[channel].tail  = 0;
    g_message_queues[channel].count = 0;
}

/* Inline memcpy (no libc) */
static void ipc_memcpy(void* dst, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < n; i++) d[i] = s[i];
}

ipc_status_t ipc_mq_enqueue(ipc_channel_handle_t channel,
                             const ipc_message_t* msg) {
    ipc_status_t vs = ipc_perm_validate_channel_handle(channel);
    if (vs != IPC_SUCCESS) return vs;
    if (!msg) return IPC_ERR_NULL_POINTER;
    if (!ipc_channel_is_valid(channel)) return IPC_ERR_INVALID_HANDLE;

    ipc_mq_ring_t* q = &g_message_queues[channel];
    if (q->count >= IPC_MESSAGE_QUEUE_DEPTH) {
        return IPC_ERR_QUEUE_FULL;
    }

    ipc_memcpy(&q->messages[q->tail], msg,
               sizeof(ipc_message_header_t) + msg->header.payload_size);
    q->tail = (q->tail + 1) % IPC_MESSAGE_QUEUE_DEPTH;
    q->count++;

    return IPC_SUCCESS;
}

ipc_status_t ipc_mq_dequeue(ipc_channel_handle_t channel,
                             ipc_message_t* out_msg, uint32_t flags) {
    ipc_status_t vs = ipc_perm_validate_channel_handle(channel);
    if (vs != IPC_SUCCESS) return vs;
    if (!out_msg) return IPC_ERR_NULL_POINTER;
    if (!ipc_channel_is_valid(channel)) return IPC_ERR_INVALID_HANDLE;

    ipc_mq_ring_t* q = &g_message_queues[channel];

    if (flags & IPC_FLAG_NONBLOCKING) {
        if (q->count == 0) return IPC_ERR_QUEUE_EMPTY;
    } else {
        /* Blocking spin-wait (will be replaced by scheduler yield in future) */
        volatile uint32_t spin = 0;
        while (q->count == 0) {
            __asm__ volatile("pause" ::: "memory");
            spin++;
            if (spin > 1000000) return IPC_ERR_TIMEOUT;
        }
    }

    ipc_memcpy(out_msg, &q->messages[q->head],
               sizeof(ipc_message_header_t) + q->messages[q->head].header.payload_size);
    q->head = (q->head + 1) % IPC_MESSAGE_QUEUE_DEPTH;
    q->count--;

    return IPC_SUCCESS;
}

uint32_t ipc_mq_pending_count(ipc_channel_handle_t channel) {
    if (channel >= IPC_MAX_CHANNELS) return 0;
    return g_message_queues[channel].count;
}

bool ipc_mq_is_full(ipc_channel_handle_t channel) {
    if (channel >= IPC_MAX_CHANNELS) return true;
    return (g_message_queues[channel].count >= IPC_MESSAGE_QUEUE_DEPTH);
}

bool ipc_mq_is_empty(ipc_channel_handle_t channel) {
    if (channel >= IPC_MAX_CHANNELS) return true;
    return (g_message_queues[channel].count == 0);
}
