/*
 * BOSPECTRA V3 — Timeline Engine Subsystem
 * kernel/media/bospectra/scheduler/timeline_engine.h
 *
 * Playback timeline authority managing current position, duration, EOS, and looping.
 */

#ifndef BOSPECTRA_V3_TIMELINE_ENGINE_H
#define BOSPECTRA_V3_TIMELINE_ENGINE_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t current_position_us;
    uint64_t next_presentation_pts_us;
    uint64_t duration_us;
    bool     eos_reached;
    bool     loop_enabled;
} BOSPECTRA_TimelineEngine;

void bospectra_timeline_engine_init(BOSPECTRA_TimelineEngine* tl);
void bospectra_timeline_engine_reset(BOSPECTRA_TimelineEngine* tl);

void bospectra_timeline_engine_set_duration(BOSPECTRA_TimelineEngine* tl, uint64_t duration_us);
void bospectra_timeline_engine_update_position(BOSPECTRA_TimelineEngine* tl, uint64_t pts_us);
void bospectra_timeline_engine_set_loop(BOSPECTRA_TimelineEngine* tl, bool loop);

#endif /* BOSPECTRA_V3_TIMELINE_ENGINE_H */
