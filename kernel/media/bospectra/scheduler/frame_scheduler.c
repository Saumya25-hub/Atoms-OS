/*
 * BOSPECTRA V3 — Frame Scheduler Implementation
 * kernel/media/bospectra/scheduler/frame_scheduler.c
 */

#include "frame_scheduler.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_SCHEDULER_EARLY_THRESHOLD_US 15000U /* 15 ms */
#define BOSPECTRA_SCHEDULER_LATE_THRESHOLD_US  40000U /* 40 ms */

void bospectra_frame_scheduler_init(BOSPECTRA_FrameSchedulerContext* ctx, uint32_t target_fps) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(BOSPECTRA_FrameSchedulerContext));
    bospectra_master_clock_init(&ctx->master_clock);
    bospectra_pts_manager_init(&ctx->pts_manager);
    bospectra_timeline_engine_init(&ctx->timeline);
    bospectra_frame_pacer_init(&ctx->pacer, target_fps);
    bospectra_log("FRAME_SCHEDULER", "BOSPECTRA V3 Frame Scheduler Initialized.");
}

void bospectra_frame_scheduler_reset(BOSPECTRA_FrameSchedulerContext* ctx) {
    if (!ctx) return;
    uint32_t fps = ctx->pacer.target_fps;
    memset(ctx, 0, sizeof(BOSPECTRA_FrameSchedulerContext));
    bospectra_master_clock_init(&ctx->master_clock);
    bospectra_pts_manager_init(&ctx->pts_manager);
    bospectra_timeline_engine_init(&ctx->timeline);
    bospectra_frame_pacer_init(&ctx->pacer, fps);
}

BOSPECTRA_FrameAction bospectra_frame_scheduler_evaluate(BOSPECTRA_FrameSchedulerContext* ctx, uint64_t frame_pts_us) {
    if (!ctx) return FRAME_ACTION_PRESENT_NOW;

    bospectra_pts_validate(&ctx->pts_manager, frame_pts_us);
    uint64_t current_media_time = bospectra_master_clock_get_media_time(&ctx->master_clock);

    /* First frame or PTS=0: sync master clock to frame and present immediately */
    if (ctx->frames_presented == 0 || frame_pts_us == 0) {
        bospectra_master_clock_seek(&ctx->master_clock, frame_pts_us);
        ctx->frames_presented++;
        bospectra_timeline_engine_update_position(&ctx->timeline, frame_pts_us);
        bospectra_frame_pacer_record_present(&ctx->pacer, frame_pts_us, frame_pts_us);
        return FRAME_ACTION_PRESENT_NOW;
    }

    /* Frame PTS significantly ahead of master clock -> Too Early */
    if (frame_pts_us > current_media_time + BOSPECTRA_SCHEDULER_EARLY_THRESHOLD_US) {
        return FRAME_ACTION_TOO_EARLY;
    }

    /* Frame PTS significantly behind master clock -> Late / Drop */
    if (current_media_time > frame_pts_us + BOSPECTRA_SCHEDULER_LATE_THRESHOLD_US) {
        ctx->frames_dropped++;
        ctx->frames_late++;
        return FRAME_ACTION_LATE_DROP;
    }

    /* Frame time matches current master clock position -> Present Now */
    ctx->frames_presented++;
    bospectra_timeline_engine_update_position(&ctx->timeline, frame_pts_us);
    bospectra_frame_pacer_record_present(&ctx->pacer, current_media_time, frame_pts_us);
    return FRAME_ACTION_PRESENT_NOW;
}
