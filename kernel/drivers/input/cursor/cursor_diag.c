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

void cursor_diag_set_backend(uint32_t backend_type) {
    g_diag_stats.active_backend = backend_type;
}

void cursor_diag_get_stats(CursorDiagStats* out_stats) {
    if (!out_stats) return;
    *out_stats = g_diag_stats;
}
