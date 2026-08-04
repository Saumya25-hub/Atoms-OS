/*
 * BOSPECTRA V3 — Session Dump Implementation
 * kernel/media/bospectra/diagnostics/session_dump.c
 */

#include "session_dump.h"
#include "../scheduler/scheduler_metrics.h"
#include "../debug/bospectra_debug.h"

extern void display_print(const char* str);

void bospectra_dump_session(const PlaybackSessionCtx* sess) {
    if (!sess) return;

    display_print("\n============= PLAYBACK SESSION DUMP =============\n");
    display_print("Session ID       : ");
    bospectra_trace_u32("ID", sess->session_id);
    display_print("Media URI        : ");
    display_print(sess->media_uri);
    display_print("\nFrames Decoded   : ");
    bospectra_trace_u32("Decoded", (uint32_t)sess->total_frames_decoded);
    display_print("Frames Rendered  : ");
    bospectra_trace_u32("Rendered", (uint32_t)sess->total_frames_rendered);
    display_print("=================================================\n\n");

    bospectra_scheduler_metrics_dump(&sess->frame_scheduler_ctx);
}
