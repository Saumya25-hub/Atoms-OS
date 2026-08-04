/*
 * BOSPECTRA V3 — Pipeline Scheduler Implementation
 * kernel/media/bospectra/pipeline/scheduler.c
 */

#include "scheduler.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

void bospectra_pipeline_scheduler_init(BOSPECTRA_PipelineScheduler* sched) {
    if (!sched) return;
    memset(sched, 0, sizeof(BOSPECTRA_PipelineScheduler));
    sched->active = true;
}

void bospectra_pipeline_scheduler_reset(BOSPECTRA_PipelineScheduler* sched) {
    if (!sched) return;
    memset(sched->stage_execution_ticks, 0, sizeof(sched->stage_execution_ticks));
    sched->total_pipeline_cycles = 0;
    sched->active = true;
}

bospectra_error_t bospectra_pipeline_scheduler_step(
    BOSPECTRA_PipelineScheduler* sched,
    void* session_ctx,
    bospectra_error_t (*demux_fn)(void* ctx),
    bospectra_error_t (*decode_fn)(void* ctx),
    bospectra_error_t (*render_fn)(void* ctx)
) {
    if (!sched || !session_ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (!sched->active) return BOSPECTRA_ERR_NOT_INITIALIZED;

    sched->total_pipeline_cycles++;

    /* 1. Demux Stage */
    if (demux_fn) {
        (void)demux_fn(session_ctx);
        sched->stage_execution_ticks[SCHEDULER_STAGE_DEMUX]++;
    }

    /* 2. Decode Stage */
    if (decode_fn) {
        (void)decode_fn(session_ctx);
        sched->stage_execution_ticks[SCHEDULER_STAGE_DECODE]++;
    }

    /* 3. Render / Present Stage */
    if (render_fn) {
        (void)render_fn(session_ctx);
        sched->stage_execution_ticks[SCHEDULER_STAGE_RENDER]++;
        sched->stage_execution_ticks[SCHEDULER_STAGE_PRESENT]++;
    }

    return BOSPECTRA_SUCCESS;
}
