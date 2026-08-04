/*
 * BOSPECTRA V3 — Timeline Engine Implementation
 * kernel/media/bospectra/scheduler/timeline_engine.c
 */

#include "timeline_engine.h"
#include "kernel/core/lib/include/string.h"

void bospectra_timeline_engine_init(BOSPECTRA_TimelineEngine* tl) {
    if (!tl) return;
    memset(tl, 0, sizeof(BOSPECTRA_TimelineEngine));
}

void bospectra_timeline_engine_reset(BOSPECTRA_TimelineEngine* tl) {
    if (!tl) return;
    memset(tl, 0, sizeof(BOSPECTRA_TimelineEngine));
}

void bospectra_timeline_engine_set_duration(BOSPECTRA_TimelineEngine* tl, uint64_t duration_us) {
    if (tl) tl->duration_us = duration_us;
}

void bospectra_timeline_engine_update_position(BOSPECTRA_TimelineEngine* tl, uint64_t pts_us) {
    if (!tl) return;
    tl->current_position_us = pts_us;
    if (tl->duration_us > 0 && pts_us >= tl->duration_us) {
        tl->eos_reached = true;
    }
}

void bospectra_timeline_engine_set_loop(BOSPECTRA_TimelineEngine* tl, bool loop) {
    if (tl) tl->loop_enabled = loop;
}
