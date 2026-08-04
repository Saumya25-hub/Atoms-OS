/*
 * BOSPECTRA V3 — Packet Queue Implementation
 * kernel/media/bospectra/pipeline/packet_queue.c
 */

#include "packet_queue.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

void bospectra_packet_queue_init(BOSPECTRA_PacketQueue* q) {
    if (!q) return;
    memset(q, 0, sizeof(BOSPECTRA_PacketQueue));
}

void bospectra_packet_queue_reset(BOSPECTRA_PacketQueue* q) {
    if (!q) return;
    bospectra_packet_queue_flush(q);
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->overflows = 0;
    q->underflows = 0;
    q->total_enqueued = 0;
    q->total_dequeued = 0;
}

void bospectra_packet_queue_flush(BOSPECTRA_PacketQueue* q) {
    if (!q) return;
    while (q->count > 0) {
        BOSPacket* pkt = q->packets[q->head];
        if (pkt) {
            bospectra_packet_pool_release(pkt);
            q->packets[q->head] = NULL;
        }
        q->head = (q->head + 1) % BOSPECTRA_PACKET_QUEUE_CAPACITY;
        q->count--;
    }
}

#include "../resource/leak_detector.h"
#include "../resource/ownership_manager.h"

bospectra_error_t bospectra_packet_queue_enqueue(BOSPECTRA_PacketQueue* q, BOSPacket* pkt) {
    if (!q || !pkt) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (q->count >= BOSPECTRA_PACKET_QUEUE_CAPACITY) {
        q->overflows++;
        return BOSPECTRA_ERR_BUFFER_OVERFLOW;
    }

    q->packets[q->tail] = pkt;
    q->tail = (q->tail + 1) % BOSPECTRA_PACKET_QUEUE_CAPACITY;
    q->count++;
    q->total_enqueued++;
    bospectra_leak_track_alloc((uint32_t)(uintptr_t)pkt);
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_packet_queue_dequeue(BOSPECTRA_PacketQueue* q, BOSPacket** out_pkt) {
    if (!q || !out_pkt) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (q->count == 0) {
        q->underflows++;
        *out_pkt = NULL;
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    *out_pkt = q->packets[q->head];
    q->packets[q->head] = NULL;
    q->head = (q->head + 1) % BOSPECTRA_PACKET_QUEUE_CAPACITY;
    q->count--;
    q->total_dequeued++;
    if (*out_pkt) {
        bospectra_leak_track_release((uint32_t)(uintptr_t)*out_pkt);
    }
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_packet_queue_peek(const BOSPECTRA_PacketQueue* q, BOSPacket** out_pkt) {
    if (!q || !out_pkt) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (q->count == 0) {
        *out_pkt = NULL;
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }
    *out_pkt = q->packets[q->head];
    return BOSPECTRA_SUCCESS;
}

uint32_t bospectra_packet_queue_get_count(const BOSPECTRA_PacketQueue* q) {
    return q ? q->count : 0;
}

bool bospectra_packet_queue_is_full(const BOSPECTRA_PacketQueue* q) {
    return q ? (q->count >= BOSPECTRA_PACKET_QUEUE_CAPACITY) : false;
}

bool bospectra_packet_queue_is_empty(const BOSPECTRA_PacketQueue* q) {
    return q ? (q->count == 0) : true;
}
