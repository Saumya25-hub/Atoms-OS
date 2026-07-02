#include "kernel/shell/rook/include/rook_debug.h"
#include "kernel/shell/rook/include/rook.h"

/*
 * ♜ ROOK ENGINE V1.0 — Debug Telemetry HUD Overlay
 */

static bool g_debug_overlay_enabled = false;

void rook_toggle_debug_overlay(bool enable) {
    g_debug_overlay_enabled = enable;
    rook_invalidate_full();
}

bool rook_is_debug_overlay_enabled(void) {
    return g_debug_overlay_enabled;
}

void rook_debug_log_event(const char* event_msg) {
    (void)event_msg;
}

void rook_debug_render_overlay(uint32_t* framebuffer, uint32_t width, uint32_t height, uint32_t stride) {
    if (!g_debug_overlay_enabled || !framebuffer) return;

    /* Draw top-left telemetry banner background box (e.g. 300x40 dark gray) */
    uint32_t box_w = (width > 350) ? 350 : width;
    uint32_t box_h = 40;
    uint32_t pitch_pixels = stride / 4;

    for (uint32_t y = 0; y < box_h && y < height; y++) {
        for (uint32_t x = 0; x < box_w; x++) {
            framebuffer[y * pitch_pixels + x] = 0xFF222222; /* Dark Gray ARGB */
        }
    }

    /* Draw green status indicator square in corner */
    for (uint32_t y = 5; y < 15; y++) {
        for (uint32_t x = 5; x < 15; x++) {
            framebuffer[y * pitch_pixels + x] = 0xFF00FF00; /* Green ARGB */
        }
    }
}
