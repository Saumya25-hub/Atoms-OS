/*
 * BOSPECTRA V3 — Scheduler Metrics Implementation
 * kernel/media/bospectra/scheduler/scheduler_metrics.c
 */

#include "scheduler_metrics.h"
#include "../debug/bospectra_debug.h"

extern void display_print(const char* str);

void bospectra_scheduler_metrics_dump(const BOSPECTRA_FrameSchedulerContext* ctx) {
    if (!ctx) return;

    display_print("\n============== FRAME SCHEDULER ==============");
    display_print("\nMaster Clock         : ");
    bospectra_trace_u32("Master Clock US", (uint32_t)ctx->master_clock.master_clock_us);
    display_print("\nPlayback Position    : ");
    bospectra_trace_u32("Position US", (uint32_t)ctx->timeline.current_position_us);

    display_print("\n\nFrame Metrics:");
    display_print("\n  Presented Frames   : ");
    bospectra_trace_u32("Presented", ctx->frames_presented);
    display_print("\n  Dropped Frames     : ");
    bospectra_trace_u32("Dropped", ctx->frames_dropped);
    display_print("\n  Late Frames        : ");
    bospectra_trace_u32("Late", ctx->frames_late);
    display_print("\n  Duplicate Frames   : ");
    bospectra_trace_u32("Duplicated", ctx->frames_duplicated);

    display_print("\n\nPacing & Clock:");
    display_print("\n  Target FPS         : ");
    bospectra_trace_u32("Target FPS", ctx->pacer.target_fps);
    display_print("\n=============================================\n\n");
}
