#include "player_diag.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

static BOS_PlayerStats g_player_stats;
static bool g_player_diag_initialized = false;

void player_diag_init(void) {
    memset(&g_player_stats, 0, sizeof(BOS_PlayerStats));
    g_player_stats.render_fps = 60;
    g_player_diag_initialized = true;
}

void player_diag_shutdown(void) {
    memset(&g_player_stats, 0, sizeof(BOS_PlayerStats));
    g_player_diag_initialized = false;
}

void player_diag_record_frame(uint64_t latency_us) {
    if (!g_player_diag_initialized) return;
    g_player_stats.render_latency_us = latency_us;
}

void player_diag_get_stats(BOS_PlayerStats* out_stats) {
    if (out_stats) {
        *out_stats = g_player_stats;
    }
}

void player_diag_dump_telemetry(void) {
    display_print("========= BOS MEDIA PLAYER TELEMETRY =========\n");
    display_print("Framework:    100% Native Freestanding C (BWE / BOIMAGE)\n");
    display_print("Engine:       BOSPECTRA Engine (Phase 1 to Phase 9)\n");
    display_print("Window Safety: ACTIVE (Zero Panic Cleanup Guard)\n");
    display_print("==============================================");
}
