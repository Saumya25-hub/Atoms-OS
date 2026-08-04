/*
 * BOSPECTRA V3 — Decode Queue Subsystem
 * kernel/media/bospectra/pipeline/decode_queue.h
 *
 * Intermediate queue feeding packets to codec drivers.
 */

#ifndef BOSPECTRA_V3_DECODE_QUEUE_H
#define BOSPECTRA_V3_DECODE_QUEUE_H

#include "packet_queue.h"

typedef struct {
    BOSPECTRA_PacketQueue packet_input_queue;
    uint32_t total_packets_submitted;
    uint32_t total_decodes_completed;
    uint32_t total_decode_failures;
    bool backpressure_active;
} BOSPECTRA_DecodeQueue;

void bospectra_decode_queue_init(BOSPECTRA_DecodeQueue* dq);
void bospectra_decode_queue_reset(BOSPECTRA_DecodeQueue* dq);

bospectra_error_t bospectra_decode_queue_submit_packet(BOSPECTRA_DecodeQueue* dq, BOSPacket* pkt);
bospectra_error_t bospectra_decode_queue_fetch_packet(BOSPECTRA_DecodeQueue* dq, BOSPacket** out_pkt);

#endif /* BOSPECTRA_V3_DECODE_QUEUE_H */
