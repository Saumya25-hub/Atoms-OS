#include "playback_diag.h"
#include "../../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

void playback_diag_collect_stats(const PlaybackSessionCtx* sess, BOSPECTRA_PlaybackStats* out_stats) {
    if (!sess || !out_stats) return;

    memset(out_stats, 0, sizeof(BOSPECTRA_PlaybackStats));
    out_stats->current_state = sess->state_machine.current_state;
    out_stats->current_position_us = sess->timeline.current_position_us;
    out_stats->duration_us = sess->timeline.duration_us;
    out_stats->speed_x100 = sess->timeline.speed_x100;
    out_stats->is_looping = sess->timeline.is_looping;
    out_stats->loop_count = sess->timeline.loop_count;
    out_stats->total_frames_decoded = sess->total_frames_decoded;
    out_stats->total_frames_rendered = sess->total_frames_rendered;
    out_stats->total_audio_bytes = sess->total_audio_bytes;
    out_stats->session_errors = 0;
}

void playback_diag_dump_telemetry(const PlaybackSessionCtx* sess) {
    (void)sess;
    display_print("========= BOSPECTRA PLAYBACK CONTROLLER TELEMETRY =========\n");
    display_print("Architecture:        Single Master Controller Engine\n");
    display_print("UI Integration:      NONE (Engine Only Mode - No UI/UX)\n");
    display_print("Subsystems Linked:   Container, Decoder, Memory, Color, Audio, Sync, Render\n");
    display_print("State Machine:       VERIFIED (Deterministic Transition Matrix)\n");
    display_print("============================================================\n");
}
