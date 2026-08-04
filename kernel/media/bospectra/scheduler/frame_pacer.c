/*
 * BOSPECTRA V3 — Frame Pacer Implementation
 * kernel/media/bospectra/scheduler/frame_pacer.c
 */

#include "frame_pacer.h"
#include "kernel/core/lib/include/string.h"

void bospectra_frame_pacer_init(BOSPECTRA_FramePacer* pacer, uint32_t target_fps) {
    if (!pacer) return;
    memset(pacer, 0, sizeof(BOSPECTRA_FramePacer));
    bospectra_frame_pacer_set_fps(pacer, target_fps > 0 ? target_fps : 30);
}

void bospectra_frame_pacer_reset(BOSPECTRA_FramePacer* pacer) {
    if (!pacer) return;
    uint32_t fps = pacer->target_fps;
    memset(pacer, 0, sizeof(BOSPECTRA_FramePacer));
    bospectra_frame_pacer_set_fps(pacer, fps > 0 ? fps : 30);
}

void bospectra_frame_pacer_set_fps(BOSPECTRA_FramePacer* pacer, uint32_t fps) {
    if (!pacer || fps == 0) return;
    pacer->target_fps = fps;
    pacer->frame_interval_us = 1000000U / fps;
}

void bospectra_frame_pacer_record_present(BOSPECTRA_FramePacer* pacer, uint64_t present_time_us, uint64_t pts_us) {
    if (!pacer) return;
    pacer->total_paced_frames++;

    if (pacer->last_presentation_us > 0) {
        uint64_t elapsed = (present_time_us > pacer->last_presentation_us) ? (present_time_us - pacer->last_presentation_us) : 0;
        int64_t diff = (int64_t)elapsed - (int64_t)pacer->frame_interval_us;
        pacer->accumulated_jitter_us += (diff < 0) ? -diff : diff;
    }
    pacer->last_presentation_us = present_time_us;
    (void)pts_us;
}
