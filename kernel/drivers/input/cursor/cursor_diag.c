/**
 * @file cursor_diag.c
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Diagnostics Implementation
 * @section PURPOSE
 * Lock-free/atomic telemetry collection for cursor rendering performance analysis.
 */

#include "cursor_diag.h"
#include <stddef.h>

static CursorDiagStats g_diag_stats;

void cursor_diag_init(void) {
    cursor_diag_reset_stats();
}

void cursor_diag_reset_stats(void) {
    g_diag_stats.frames_rendered = 0;
    g_diag_stats.cursor_updates = 0;
    g_diag_stats.cursor_moves = 0;
    g_diag_stats.theme_changes = 0;
    g_diag_stats.animation_frames = 0;
    g_diag_stats.dirty_regions_updated = 0;
    g_diag_stats.fallback_count = 0;
    g_diag_stats.active_backend = 2; /* Default software fallback */
    g_diag_stats.total_render_time_us = 0;
    g_diag_stats.max_render_time_us = 0;
    g_diag_stats.last_render_time_us = 0;
}

void cursor_diag_log_update(bool moved) {
    g_diag_stats.cursor_updates++;
    if (moved) {
        g_diag_stats.cursor_moves++;
    }
}

void cursor_diag_log_theme_change(void) {
    g_diag_stats.theme_changes++;
}

void cursor_diag_log_anim_frame(void) {
    g_diag_stats.animation_frames++;
}

void cursor_diag_log_render(uint32_t time_us, bool dirty_updated) {
    g_diag_stats.frames_rendered++;
    g_diag_stats.last_render_time_us = time_us;
    g_diag_stats.total_render_time_us += time_us;
    if (time_us > g_diag_stats.max_render_time_us) {
        g_diag_stats.max_render_time_us = time_us;
    }
    if (dirty_updated) {
        g_diag_stats.dirty_regions_updated++;
    }
}

void cursor_diag_log_fallback(void) {
    g_diag_stats.fallback_count++;
}

extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);
extern void display_print_hex(uint64_t val);

void cursor_diag_set_backend(uint32_t backend_type) {
    g_diag_stats.active_backend = backend_type;
}

void cursor_diag_get_stats(CursorDiagStats* out_stats) {
    if (!out_stats) return;
    *out_stats = g_diag_stats;
}

void cursor_diag_dump_autopsy(void) {
    display_print("\n==========================\n");
    display_print("  CURSOR DIAGNOSTICS AUTOPSY \n");
    display_print("==========================\n");
    display_print(" Updates             : "); display_print_dec(g_diag_stats.cursor_updates); display_print("\n");
    display_print(" Moves               : "); display_print_dec(g_diag_stats.cursor_moves); display_print("\n");
    display_print(" Active Backend      : "); display_print(g_diag_stats.active_backend == 1 ? "HARDWARE" : "SOFTWARE"); display_print("\n");
    display_print(" Hardware Overlay    : "); display_print(g_diag_stats.active_backend == 1 ? "YES" : "NO"); display_print("\n");
    display_print(" Fallback Count      : "); display_print_dec(g_diag_stats.fallback_count); display_print("\n");
    display_print(" Last Render Time    : "); display_print_dec(g_diag_stats.last_render_time_us); display_print(" us\n");
    display_print(" Max Render Time     : "); display_print_dec(g_diag_stats.max_render_time_us); display_print(" us\n");
    display_print("==========================\n\n");
}
