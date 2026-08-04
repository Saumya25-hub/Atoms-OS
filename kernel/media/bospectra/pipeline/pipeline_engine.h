/*
 * BOSPECTRA V3 — Pipeline Engine Subsystem
 * kernel/media/bospectra/pipeline/pipeline_engine.h
 *
 * Execution brain for queue management, state transitions, and stage orchestration.
 */

#ifndef BOSPECTRA_V3_PIPELINE_ENGINE_H
#define BOSPECTRA_V3_PIPELINE_ENGINE_H

#include "packet_queue.h"
#include "decode_queue.h"
#include "frame_queue.h"
#include "renderer_queue.h"
#include "scheduler.h"

typedef enum {
    PIPELINE_STATE_UNINITIALIZED = 0,
    PIPELINE_STATE_STOPPED,
    PIPELINE_STATE_RUNNING,
    PIPELINE_STATE_PAUSED,
    PIPELINE_STATE_DRAINING,
    PIPELINE_STATE_ERROR
} BOSPECTRA_PipelineState;

typedef struct {
    BOSPECTRA_PipelineState     state;
    BOSPECTRA_PacketQueue       demux_packet_queue;
    BOSPECTRA_DecodeQueue       decoder_input_queue;
    BOSPECTRA_FrameQueue        decoded_frame_queue;
    BOSPECTRA_RendererQueue     renderer_output_queue;
    BOSPECTRA_PipelineScheduler scheduler;
} BOSPECTRA_PipelineContext;

void bospectra_pipeline_context_init(BOSPECTRA_PipelineContext* ctx);
void bospectra_pipeline_context_reset(BOSPECTRA_PipelineContext* ctx);

bospectra_error_t bospectra_pipeline_start(BOSPECTRA_PipelineContext* ctx);
bospectra_error_t bospectra_pipeline_pause(BOSPECTRA_PipelineContext* ctx);
bospectra_error_t bospectra_pipeline_resume(BOSPECTRA_PipelineContext* ctx);
bospectra_error_t bospectra_pipeline_stop(BOSPECTRA_PipelineContext* ctx);
bospectra_error_t bospectra_pipeline_flush(BOSPECTRA_PipelineContext* ctx);
bospectra_error_t bospectra_pipeline_drain(BOSPECTRA_PipelineContext* ctx);
bospectra_error_t bospectra_pipeline_recover(BOSPECTRA_PipelineContext* ctx);

#endif /* BOSPECTRA_V3_PIPELINE_ENGINE_H */
