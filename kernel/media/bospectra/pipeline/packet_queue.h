/*
 * BOSPECTRA V3 — Packet Queue Subsystem
 * kernel/media/bospectra/pipeline/packet_queue.h
 *
 * SPSC lock-free ring buffer for demuxed packets.
 * Includes PTS/DTS metadata, overflow/underflow tracking, zero per-frame allocation.
 */

#ifndef BOSPECTRA_V3_PACKET_QUEUE_H
#define BOSPECTRA_V3_PACKET_QUEUE_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include "../frame_memory/packet_pool/packet_pool.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BOSPECTRA_PACKET_QUEUE_CAPACITY 32U

typedef struct {
    BOSPacket* packets[BOSPECTRA_PACKET_QUEUE_CAPACITY];
    uint32_t   head;
    uint32_t   tail;
    uint32_t   count;
    uint32_t   overflows;
    uint32_t   underflows;
    uint32_t   total_enqueued;
    uint32_t   total_dequeued;
} BOSPECTRA_PacketQueue;

void bospectra_packet_queue_init(BOSPECTRA_PacketQueue* q);
void bospectra_packet_queue_reset(BOSPECTRA_PacketQueue* q);
void bospectra_packet_queue_flush(BOSPECTRA_PacketQueue* q);

bospectra_error_t bospectra_packet_queue_enqueue(BOSPECTRA_PacketQueue* q, BOSPacket* pkt);
bospectra_error_t bospectra_packet_queue_dequeue(BOSPECTRA_PacketQueue* q, BOSPacket** out_pkt);
bospectra_error_t bospectra_packet_queue_peek(const BOSPECTRA_PacketQueue* q, BOSPacket** out_pkt);

uint32_t bospectra_packet_queue_get_count(const BOSPECTRA_PacketQueue* q);
bool     bospectra_packet_queue_is_full(const BOSPECTRA_PacketQueue* q);
bool     bospectra_packet_queue_is_empty(const BOSPECTRA_PacketQueue* q);

#endif /* BOSPECTRA_V3_PACKET_QUEUE_H */
