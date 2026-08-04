/*
 * BOSPECTRA V3 — Frame Queue Implementation
 * kernel/media/bospectra/pipeline/frame_queue.c
 */

#include "frame_queue.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

void bospectra_frame_queue_init(BOSPECTRA_FrameQueue* fq) {
    if (!fq) return;
    memset(fq, 0, sizeof(BOSPECTRA_FrameQueue));
}

void bospectra_frame_queue_reset(BOSPECTRA_FrameQueue* fq) {
    if (!fq) return;
    bospectra_frame_queue_flush(fq);
    fq->head = 0;
    fq->tail = 0;
    fq->count = 0;
    fq->overflows = 0;
    fq->underflows = 0;
    fq->total_enqueued = 0;
    fq->total_dequeued = 0;
}

void bospectra_frame_queue_flush(BOSPECTRA_FrameQueue* fq) {
    if (!fq) return;
    while (fq->count > 0) {
        BOSFrame* f = fq->frames[fq->head];
        if (f) {
            bospectra_frame_release(f);
            fq->frames[fq->head] = NULL;
        }
        fq->head = (fq->head + 1) % BOSPECTRA_FRAME_QUEUE_CAPACITY;
        fq->count--;
    }
}

bospectra_error_t bospectra_frame_queue_enqueue(BOSPECTRA_FrameQueue* fq, BOSFrame* frame) {
    if (!fq || !frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (fq->count >= BOSPECTRA_FRAME_QUEUE_CAPACITY) {
        fq->overflows++;
        return BOSPECTRA_ERR_BUFFER_OVERFLOW;
    }

    fq->frames[fq->tail] = frame;
    fq->tail = (fq->tail + 1) % BOSPECTRA_FRAME_QUEUE_CAPACITY;
    fq->count++;
    fq->total_enqueued++;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_frame_queue_dequeue(BOSPECTRA_FrameQueue* fq, BOSFrame** out_frame) {
    if (!fq || !out_frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (fq->count == 0) {
        fq->underflows++;
        *out_frame = NULL;
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    *out_frame = fq->frames[fq->head];
    fq->frames[fq->head] = NULL;
    fq->head = (fq->head + 1) % BOSPECTRA_FRAME_QUEUE_CAPACITY;
    fq->count--;
    fq->total_dequeued++;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_frame_queue_peek(const BOSPECTRA_FrameQueue* fq, BOSFrame** out_frame) {
    if (!fq || !out_frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (fq->count == 0) {
        *out_frame = NULL;
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }
    *out_frame = fq->frames[fq->head];
    return BOSPECTRA_SUCCESS;
}

uint32_t bospectra_frame_queue_get_count(const BOSPECTRA_FrameQueue* fq) {
    return fq ? fq->count : 0;
}

bool bospectra_frame_queue_is_full(const BOSPECTRA_FrameQueue* fq) {
    return fq ? (fq->count >= BOSPECTRA_FRAME_QUEUE_CAPACITY) : false;
}

bool bospectra_frame_queue_is_empty(const BOSPECTRA_FrameQueue* fq) {
    return fq ? (fq->count == 0) : true;
}
