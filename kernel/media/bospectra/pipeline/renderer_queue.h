/*
 * BOSPECTRA V3 — Renderer Queue Subsystem
 * kernel/media/bospectra/pipeline/renderer_queue.h
 *
 * Queue managing presentation ordering, vsync pacing, and dropped frame accounting.
 */

#ifndef BOSPECTRA_V3_RENDERER_QUEUE_H
#define BOSPECTRA_V3_RENDERER_QUEUE_H

#include "frame_queue.h"

typedef struct {
    BOSPECTRA_FrameQueue frame_present_queue;
    uint32_t total_frames_submitted;
    uint32_t total_frames_presented;
    uint32_t total_frames_dropped;
    uint64_t last_presented_pts_us;
} BOSPECTRA_RendererQueue;

void bospectra_renderer_queue_init(BOSPECTRA_RendererQueue* rq);
void bospectra_renderer_queue_reset(BOSPECTRA_RendererQueue* rq);

bospectra_error_t bospectra_renderer_queue_submit_frame(BOSPECTRA_RendererQueue* rq, BOSFrame* frame);
bospectra_error_t bospectra_renderer_queue_fetch_frame(BOSPECTRA_RendererQueue* rq, BOSFrame** out_frame);
void bospectra_renderer_queue_record_drop(BOSPECTRA_RendererQueue* rq);

#endif /* BOSPECTRA_V3_RENDERER_QUEUE_H */
