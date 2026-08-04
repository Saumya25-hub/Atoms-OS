/*
 * BOSPECTRA V3 — Frame Scheduler Subsystem
 * kernel/media/bospectra/scheduler/frame_scheduler.h
 *
 * Evaluates decoded frame presentation timestamps against master clock time.
 */

#ifndef BOSPECTRA_V3_FRAME_SCHEDULER_H
#define BOSPECTRA_V3_FRAME_SCHEDULER_H

#include "scheduler_clock.h"
#include "pts_manager.h"
#include "timeline_engine.h"
#include "frame_pacer.h"

typedef enum {
    FRAME_ACTION_TOO_EARLY = 0,
    FRAME_ACTION_PRESENT_NOW,
    FRAME_ACTION_LATE_DROP,
    FRAME_ACTION_DUPLICATE
} BOSPECTRA_FrameAction;

typedef struct {
    BOSPECTRA_MasterClock    master_clock;
    BOSPECTRA_PTSManager     pts_manager;
    BOSPECTRA_TimelineEngine timeline;
    BOSPECTRA_FramePacer     pacer;
    uint32_t                 frames_presented;
    uint32_t                 frames_dropped;
    uint32_t                 frames_duplicated;
    uint32_t                 frames_late;
} BOSPECTRA_FrameSchedulerContext;

void bospectra_frame_scheduler_init(BOSPECTRA_FrameSchedulerContext* ctx, uint32_t target_fps);
void bospectra_frame_scheduler_reset(BOSPECTRA_FrameSchedulerContext* ctx);

BOSPECTRA_FrameAction bospectra_frame_scheduler_evaluate(BOSPECTRA_FrameSchedulerContext* ctx, uint64_t frame_pts_us);

#endif /* BOSPECTRA_V3_FRAME_SCHEDULER_H */
