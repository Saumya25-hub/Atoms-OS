/*
 * BOSPECTRA V3 — Queue Metrics Diagnostics Implementation
 * kernel/media/bospectra/pipeline/queue_metrics.c
 */

#include "queue_metrics.h"
#include "../debug/bospectra_debug.h"

extern void display_print(const char* str);

void bospectra_queue_metrics_dump(const BOSPECTRA_PipelineContext* ctx) {
    if (!ctx) return;

    display_print("\n============= PIPELINE METRICS =============");
    display_print("\nPipeline State       : ");
    switch (ctx->state) {
        case PIPELINE_STATE_RUNNING:  display_print("RUNNING"); break;
        case PIPELINE_STATE_PAUSED:   display_print("PAUSED"); break;
        case PIPELINE_STATE_STOPPED:  display_print("STOPPED"); break;
        case PIPELINE_STATE_DRAINING: display_print("DRAINING"); break;
        default:                      display_print("UNINITIALIZED"); break;
    }

    display_print("\n\nPacket Queue:");
    display_print("\n  Depth              : ");
    bospectra_trace_u32("Packet Queue Depth", ctx->demux_packet_queue.count);
    display_print("\n  Overflows          : ");
    bospectra_trace_u32("Overflows", ctx->demux_packet_queue.overflows);
    display_print("\n  Underflows         : ");
    bospectra_trace_u32("Underflows", ctx->demux_packet_queue.underflows);

    display_print("\n\nFrame Queue:");
    display_print("\n  Depth              : ");
    bospectra_trace_u32("Frame Queue Depth", ctx->decoded_frame_queue.count);

    display_print("\n\nRenderer Queue:");
    display_print("\n  Submitted          : ");
    bospectra_trace_u32("Submitted", ctx->renderer_output_queue.total_frames_submitted);
    display_print("\n  Presented          : ");
    bospectra_trace_u32("Presented", ctx->renderer_output_queue.total_frames_presented);
    display_print("\n  Dropped            : ");
    bospectra_trace_u32("Dropped", ctx->renderer_output_queue.total_frames_dropped);
    display_print("\n============================================\n\n");
}
