/*
 * BOSPECTRA V3 — Display Scheduler Subsystem
 * kernel/media/bospectra/scheduler/display_scheduler.h
 *
 * Exclusive module authorized to present rendered frames to display surfaces.
 */

#ifndef BOSPECTRA_V3_DISPLAY_SCHEDULER_H
#define BOSPECTRA_V3_DISPLAY_SCHEDULER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include "../render/include/bospectra_render.h"

void bospectra_display_scheduler_init(void);
void bospectra_display_scheduler_shutdown(void);

bospectra_error_t bospectra_display_scheduler_present(
    bospectra_render_session_id_t render_id,
    const BOSFrame* frame,
    int32_t x, int32_t y, int32_t w, int32_t h
);

#endif /* BOSPECTRA_V3_DISPLAY_SCHEDULER_H */
