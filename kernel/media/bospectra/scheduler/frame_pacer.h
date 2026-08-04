/*
 * BOSPECTRA V3 — Frame Pacer Subsystem
 * kernel/media/bospectra/scheduler/frame_pacer.h
 *
 * Frame interval calculator, jitter measurement engine, and catch-up/slow-down timing pacer.
 */

#ifndef BOSPECTRA_V3_FRAME_PACER_H
#define BOSPECTRA_V3_FRAME_PACER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t target_fps;            /* 24, 25, 30, 60, 120 */
    uint64_t frame_interval_us;    /* Microseconds per frame */
    uint64_t last_presentation_us;
    int64_t  accumulated_jitter_us;
    uint32_t total_paced_frames;
} BOSPECTRA_FramePacer;

void bospectra_frame_pacer_init(BOSPECTRA_FramePacer* pacer, uint32_t target_fps);
void bospectra_frame_pacer_reset(BOSPECTRA_FramePacer* pacer);

void bospectra_frame_pacer_set_fps(BOSPECTRA_FramePacer* pacer, uint32_t fps);
void bospectra_frame_pacer_record_present(BOSPECTRA_FramePacer* pacer, uint64_t present_time_us, uint64_t pts_us);

#endif /* BOSPECTRA_V3_FRAME_PACER_H */
