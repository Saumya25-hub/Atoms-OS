/*
 * BOSPECTRA V3 — Decode Queue Implementation
 * kernel/media/bospectra/pipeline/decode_queue.c
 */

#include "decode_queue.h"
#include "kernel/core/lib/include/string.h"

void bospectra_decode_queue_init(BOSPECTRA_DecodeQueue* dq) {
    if (!dq) return;
    memset(dq, 0, sizeof(BOSPECTRA_DecodeQueue));
    bospectra_packet_queue_init(&dq->packet_input_queue);
}

void bospectra_decode_queue_reset(BOSPECTRA_DecodeQueue* dq) {
    if (!dq) return;
    bospectra_packet_queue_reset(&dq->packet_input_queue);
    dq->total_packets_submitted = 0;
    dq->total_decodes_completed = 0;
    dq->total_decode_failures = 0;
    dq->backpressure_active = false;
}

bospectra_error_t bospectra_decode_queue_submit_packet(BOSPECTRA_DecodeQueue* dq, BOSPacket* pkt) {
    if (!dq || !pkt) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    bospectra_error_t err = bospectra_packet_queue_enqueue(&dq->packet_input_queue, pkt);
    if (err == BOSPECTRA_SUCCESS) {
        dq->total_packets_submitted++;
        if (bospectra_packet_queue_is_full(&dq->packet_input_queue)) {
            dq->backpressure_active = true;
        }
    }
    return err;
}

bospectra_error_t bospectra_decode_queue_fetch_packet(BOSPECTRA_DecodeQueue* dq, BOSPacket** out_pkt) {
    if (!dq || !out_pkt) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    bospectra_error_t err = bospectra_packet_queue_dequeue(&dq->packet_input_queue, out_pkt);
    if (err == BOSPECTRA_SUCCESS) {
        if (!bospectra_packet_queue_is_full(&dq->packet_input_queue)) {
            dq->backpressure_active = false;
        }
    }
    return err;
}
