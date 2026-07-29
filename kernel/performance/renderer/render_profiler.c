/**
 * @file render_profiler.c
 * @brief Renderer Pipeline Stage Profiler Delegate
 */

#include "../include/profiler.h"

#if BOS_ENABLE_PROFILER

void bos_prof_render_mark_stage(BOS_ProfStage render_stage) {
    bos_profiler_stage_begin(render_stage);
}

#endif /* BOS_ENABLE_PROFILER */
