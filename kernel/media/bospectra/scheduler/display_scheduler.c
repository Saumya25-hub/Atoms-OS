/*
 * BOSPECTRA V3 — Display Scheduler Implementation
 * kernel/media/bospectra/scheduler/display_scheduler.c
 */

#include "display_scheduler.h"
#include "../debug/bospectra_debug.h"

static bool g_display_scheduler_initialized = false;

void bospectra_display_scheduler_init(void) {
    g_display_scheduler_initialized = true;
    bospectra_log("DISPLAY_SCHEDULER", "BOSPECTRA V3 Display Scheduler Initialized.");
}

void bospectra_display_scheduler_shutdown(void) {
    g_display_scheduler_initialized = false;
}

bospectra_error_t bospectra_display_scheduler_present(
    bospectra_render_session_id_t render_id,
    const BOSFrame* frame,
    int32_t x, int32_t y, int32_t w, int32_t h
) {
    if (!g_display_scheduler_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    return BOSPECTRA_Render_PresentFrame(render_id, frame, x, y, w, h);
}
