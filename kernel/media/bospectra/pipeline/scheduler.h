/*
 * BOSPECTRA V3 — Pipeline Scheduler Subsystem
 * kernel/media/bospectra/pipeline/scheduler.h
 *
 * Master multimedia scheduler orchestrating pipeline stage execution.
 */

#ifndef BOSPECTRA_V3_PIPELINE_SCHEDULER_H
#define BOSPECTRA_V3_PIPELINE_SCHEDULER_H

#include "packet_queue.h"
#include "decode_queue.h"
#include "frame_queue.h"
#include "renderer_queue.h"

typedef enum {
    SCHEDULER_STAGE_DEMUX = 0,
    SCHEDULER_STAGE_DECODE,
    SCHEDULER_STAGE_RENDER,
    SCHEDULER_STAGE_PRESENT,
    SCHEDULER_STAGE_COUNT
} BOSPECTRA_SchedulerStage;

typedef struct {
    uint32_t stage_execution_ticks[SCHEDULER_STAGE_COUNT];
    uint32_t total_pipeline_cycles;
    bool active;
} BOSPECTRA_PipelineScheduler;

void bospectra_pipeline_scheduler_init(BOSPECTRA_PipelineScheduler* sched);
void bospectra_pipeline_scheduler_reset(BOSPECTRA_PipelineScheduler* sched);

bospectra_error_t bospectra_pipeline_scheduler_step(
    BOSPECTRA_PipelineScheduler* sched,
    void* session_ctx,
    bospectra_error_t (*demux_fn)(void* ctx),
    bospectra_error_t (*decode_fn)(void* ctx),
    bospectra_error_t (*render_fn)(void* ctx)
);

#endif /* BOSPECTRA_V3_PIPELINE_SCHEDULER_H */
