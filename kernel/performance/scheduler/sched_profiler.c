/**
 * @file sched_profiler.c
 * @brief Scheduler Task & Switch Profiler Delegate
 */

#include "../include/profiler.h"

#if BOS_ENABLE_PROFILER

static uint64_t s_sched_cycles_accum = 0;
static uint32_t s_task_switches_accum = 0;

void bos_prof_sched_record_switch(uint64_t cycles_spent) {
    s_sched_cycles_accum += cycles_spent;
    s_task_switches_accum++;
}

#endif /* BOS_ENABLE_PROFILER */
