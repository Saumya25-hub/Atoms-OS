#include "frame_scheduler.h"

static bool g_scheduler_initialized = false;

void frame_scheduler_init(void) {
    g_scheduler_initialized = true;
}

void frame_scheduler_shutdown(void) {
    g_scheduler_initialized = false;
}

bospectra_sync_decision_t frame_scheduler_evaluate(uint64_t frame_pts_us, uint64_t master_clock_us) {
    if (!g_scheduler_initialized) {
        return BOSPECTRA_SYNC_DECISION_PRESENT_IMMEDIATELY;
    }

    int64_t delta_us = (int64_t)frame_pts_us - (int64_t)master_clock_us;

    // Late Frame (>40ms behind master clock) -> DROP to catch up
    if (delta_us < -BOSPECTRA_SYNC_LATE_THRESHOLD_US) {
        return BOSPECTRA_SYNC_DECISION_DROP_FRAME;
    }

    // On-Time Window (-40ms to +10ms relative to master clock) -> PRESENT IMMEDIATELY
    if (delta_us >= -BOSPECTRA_SYNC_LATE_THRESHOLD_US && delta_us <= BOSPECTRA_SYNC_EARLY_THRESHOLD_US) {
        return BOSPECTRA_SYNC_DECISION_PRESENT_IMMEDIATELY;
    }

    // Future Frame (+10ms to +500ms ahead of master clock) -> WAIT
    if (delta_us > BOSPECTRA_SYNC_EARLY_THRESHOLD_US && delta_us <= BOSPECTRA_SYNC_FUTURE_BOUND_US) {
        return BOSPECTRA_SYNC_DECISION_WAIT;
    }

    // Far future PTS (> 500ms) -> Present immediately / resync
    return BOSPECTRA_SYNC_DECISION_PRESENT_IMMEDIATELY;
}
