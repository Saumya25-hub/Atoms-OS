/*
 * BOSPECTRA V3 — Pipeline Engine Implementation
 * kernel/media/bospectra/pipeline/pipeline_engine.c
 */

#include "pipeline_engine.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

void bospectra_pipeline_context_init(BOSPECTRA_PipelineContext* ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(BOSPECTRA_PipelineContext));
    bospectra_packet_queue_init(&ctx->demux_packet_queue);
    bospectra_decode_queue_init(&ctx->decoder_input_queue);
    bospectra_frame_queue_init(&ctx->decoded_frame_queue);
    bospectra_renderer_queue_init(&ctx->renderer_output_queue);
    bospectra_pipeline_scheduler_init(&ctx->scheduler);
    ctx->state = PIPELINE_STATE_STOPPED;
    bospectra_log("PIPELINE_ENGINE", "BOSPECTRA V3 Pipeline Context Initialized.");
}

void bospectra_pipeline_context_reset(BOSPECTRA_PipelineContext* ctx) {
    if (!ctx) return;
    bospectra_packet_queue_reset(&ctx->demux_packet_queue);
    bospectra_decode_queue_reset(&ctx->decoder_input_queue);
    bospectra_frame_queue_reset(&ctx->decoded_frame_queue);
    bospectra_renderer_queue_reset(&ctx->renderer_output_queue);
    bospectra_pipeline_scheduler_reset(&ctx->scheduler);
    ctx->state = PIPELINE_STATE_STOPPED;
}

bospectra_error_t bospectra_pipeline_start(BOSPECTRA_PipelineContext* ctx) {
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    ctx->state = PIPELINE_STATE_RUNNING;
    bospectra_log("PIPELINE_ENGINE", "Pipeline State -> RUNNING");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_pipeline_pause(BOSPECTRA_PipelineContext* ctx) {
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (ctx->state != PIPELINE_STATE_RUNNING) return BOSPECTRA_ERR_STATE_INVALID;
    ctx->state = PIPELINE_STATE_PAUSED;
    bospectra_log("PIPELINE_ENGINE", "Pipeline State -> PAUSED");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_pipeline_resume(BOSPECTRA_PipelineContext* ctx) {
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (ctx->state != PIPELINE_STATE_PAUSED) return BOSPECTRA_ERR_STATE_INVALID;
    ctx->state = PIPELINE_STATE_RUNNING;
    bospectra_log("PIPELINE_ENGINE", "Pipeline State -> RUNNING (Resumed)");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_pipeline_stop(BOSPECTRA_PipelineContext* ctx) {
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    bospectra_pipeline_flush(ctx);
    ctx->state = PIPELINE_STATE_STOPPED;
    bospectra_log("PIPELINE_ENGINE", "Pipeline State -> STOPPED");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_pipeline_flush(BOSPECTRA_PipelineContext* ctx) {
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    bospectra_packet_queue_flush(&ctx->demux_packet_queue);
    bospectra_decode_queue_reset(&ctx->decoder_input_queue);
    bospectra_frame_queue_flush(&ctx->decoded_frame_queue);
    bospectra_renderer_queue_reset(&ctx->renderer_output_queue);
    bospectra_log("PIPELINE_ENGINE", "Pipeline Queues Flushed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_pipeline_drain(BOSPECTRA_PipelineContext* ctx) {
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    ctx->state = PIPELINE_STATE_DRAINING;
    bospectra_log("PIPELINE_ENGINE", "Pipeline State -> DRAINING");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_pipeline_recover(BOSPECTRA_PipelineContext* ctx) {
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    bospectra_pipeline_flush(ctx);
    ctx->state = PIPELINE_STATE_RUNNING;
    bospectra_log("PIPELINE_ENGINE", "Pipeline Recovered to RUNNING");
    return BOSPECTRA_SUCCESS;
}
