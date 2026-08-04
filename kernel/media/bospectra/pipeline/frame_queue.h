/*
 * BOSPECTRA V3 — Frame Queue Subsystem
 * kernel/media/bospectra/pipeline/frame_queue.h
 *
 * Decoded frame queue using FramePool memory structures with zero per-frame allocation.
 */

#ifndef BOSPECTRA_V3_FRAME_QUEUE_H
#define BOSPECTRA_V3_FRAME_QUEUE_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include "../frame_memory/include/bospectra_frame.h"
#include "../frame_memory/frame_pool/frame_pool.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BOSPECTRA_FRAME_QUEUE_CAPACITY 16U

typedef struct {
    BOSFrame* frames[BOSPECTRA_FRAME_QUEUE_CAPACITY];
    uint32_t  head;
    uint32_t  tail;
    uint32_t  count;
    uint32_t  overflows;
    uint32_t  underflows;
    uint32_t  total_enqueued;
    uint32_t  total_dequeued;
} BOSPECTRA_FrameQueue;

void bospectra_frame_queue_init(BOSPECTRA_FrameQueue* fq);
void bospectra_frame_queue_reset(BOSPECTRA_FrameQueue* fq);
void bospectra_frame_queue_flush(BOSPECTRA_FrameQueue* fq);

bospectra_error_t bospectra_frame_queue_enqueue(BOSPECTRA_FrameQueue* fq, BOSFrame* frame);
bospectra_error_t bospectra_frame_queue_dequeue(BOSPECTRA_FrameQueue* fq, BOSFrame** out_frame);
bospectra_error_t bospectra_frame_queue_peek(const BOSPECTRA_FrameQueue* fq, BOSFrame** out_frame);

uint32_t bospectra_frame_queue_get_count(const BOSPECTRA_FrameQueue* fq);
bool     bospectra_frame_queue_is_full(const BOSPECTRA_FrameQueue* fq);
bool     bospectra_frame_queue_is_empty(const BOSPECTRA_FrameQueue* fq);

#endif /* BOSPECTRA_V3_FRAME_QUEUE_H */
