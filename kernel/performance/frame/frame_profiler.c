/**
 * @file frame_profiler.c
 * @brief Frame Pipeline Stage Profiler Delegate
 */

#include "../include/profiler.h"

#if BOS_ENABLE_PROFILER

static BOS_FrameMetrics* s_active_frame = 0;

void bos_prof_frame_submodule_begin(BOS_FrameMetrics* frame) {
    s_active_frame = frame;
    if (frame) {
        frame->stage_tsc[BOS_PROF_STAGE_FRAME_START] = frame->start_tsc;
    }
}

void bos_prof_frame_submodule_end(BOS_FrameMetrics* frame) {
    if (frame) {
        frame->stage_tsc[BOS_PROF_STAGE_FRAME_END] = frame->end_tsc;
    }
    s_active_frame = 0;
}

void bos_profiler_stage_begin(BOS_ProfStage stage) {
    if (!s_active_frame || stage >= BOS_PROF_STAGE_COUNT) return;
    s_active_frame->stage_tsc[stage] = bos_profiler_rdtsc();
}

void bos_profiler_stage_end(BOS_ProfStage stage) {
    (void)stage;
    /* Stage timing captured via next stage start or frame end */
}

#endif /* BOS_ENABLE_PROFILER */
