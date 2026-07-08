#ifndef ATOMS_OS_INPUT_CURSOR_DIAG_H
#define ATOMS_OS_INPUT_CURSOR_DIAG_H

/**
 * @file cursor_diag.h
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Diagnostics & Telemetry
 * @section PURPOSE
 * Real-time monitoring of cursor presentation performance: frame counts, render times,
 * active backends, dirty region repaints, and software fallback activations.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Diagnostics Telemetry Struct --- */
typedef struct {
    uint64_t frames_rendered;
    uint64_t cursor_updates;
    uint64_t cursor_moves;
    uint64_t theme_changes;
    uint64_t animation_frames;
    uint64_t dirty_regions_updated;
    uint64_t fallback_count;
    uint32_t active_backend;      /* 0 = None, 1 = Hardware, 2 = Software */
    uint64_t total_render_time_us;
    uint32_t max_render_time_us;
    uint32_t last_render_time_us;
} CursorDiagStats;

/* --- Lifecycle & Initialization --- */
void cursor_diag_init(void);
void cursor_diag_reset_stats(void);

/* --- Telemetry Logging APIs --- */
void cursor_diag_log_update(bool moved);
void cursor_diag_log_theme_change(void);
void cursor_diag_log_anim_frame(void);
void cursor_diag_log_render(uint32_t time_us, bool dirty_updated);
void cursor_diag_log_fallback(void);
void cursor_diag_set_backend(uint32_t backend_type);

/* --- Telemetry Retrieval --- */
void cursor_diag_get_stats(CursorDiagStats* out_stats);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_INPUT_CURSOR_DIAG_H
