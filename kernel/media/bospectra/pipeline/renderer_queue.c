/*
 * BOSPECTRA V3 — Renderer Queue Implementation
 * kernel/media/bospectra/pipeline/renderer_queue.c
 */

#include "renderer_queue.h"
#include "kernel/core/lib/include/string.h"

void bospectra_renderer_queue_init(BOSPECTRA_RendererQueue* rq) {
    if (!rq) return;
    memset(rq, 0, sizeof(BOSPECTRA_RendererQueue));
    bospectra_frame_queue_init(&rq->frame_present_queue);
}

void bospectra_renderer_queue_reset(BOSPECTRA_RendererQueue* rq) {
    if (!rq) return;
    bospectra_frame_queue_reset(&rq->frame_present_queue);
    rq->total_frames_submitted = 0;
    rq->total_frames_presented = 0;
    rq->total_frames_dropped = 0;
    rq->last_presented_pts_us = 0;
}

bospectra_error_t bospectra_renderer_queue_submit_frame(BOSPECTRA_RendererQueue* rq, BOSFrame* frame) {
    if (!rq || !frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    bospectra_error_t err = bospectra_frame_queue_enqueue(&rq->frame_present_queue, frame);
    if (err == BOSPECTRA_SUCCESS) {
        rq->total_frames_submitted++;
    } else {
        rq->total_frames_dropped++;
    }
    return err;
}

bospectra_error_t bospectra_renderer_queue_fetch_frame(BOSPECTRA_RendererQueue* rq, BOSFrame** out_frame) {
    if (!rq || !out_frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    bospectra_error_t err = bospectra_frame_queue_dequeue(&rq->frame_present_queue, out_frame);
    if (err == BOSPECTRA_SUCCESS && *out_frame) {
        rq->total_frames_presented++;
        rq->last_presented_pts_us = (*out_frame)->pts;
    }
    return err;
}

void bospectra_renderer_queue_record_drop(BOSPECTRA_RendererQueue* rq) {
    if (rq) rq->total_frames_dropped++;
}
