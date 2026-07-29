/**
 * @file present_profiler.c
 * @brief VRAM Present & Swap Pipeline Profiler Delegate
 */

#include "../include/profiler.h"

#if BOS_ENABLE_PROFILER

void bos_prof_present_mark_begin(void) {
    bos_profiler_stage_begin(BOS_PROF_STAGE_PRESENT);
}

void bos_prof_present_mark_end(void) {
    bos_profiler_stage_end(BOS_PROF_STAGE_PRESENT);
}

#endif /* BOS_ENABLE_PROFILER */
