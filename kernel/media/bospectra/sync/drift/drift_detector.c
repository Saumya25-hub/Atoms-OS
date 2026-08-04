#include "drift_detector.h"
#include "kernel/core/lib/include/string.h"

static int64_t g_current_drift_us = 0;
static int64_t g_avg_drift_us = 0;
static int64_t g_max_drift_us = 0;
static bool    g_drift_detector_initialized = false;

void drift_detector_init(void) {
    g_current_drift_us = 0;
    g_avg_drift_us = 0;
    g_max_drift_us = 0;
    g_drift_detector_initialized = true;
}

void drift_detector_shutdown(void) {
    g_drift_detector_initialized = false;
}

void drift_detector_reset(void) {
    g_current_drift_us = 0;
    g_avg_drift_us = 0;
    g_max_drift_us = 0;
}

void drift_detector_update(uint64_t audio_pts_us, uint64_t video_pts_us) {
    if (!g_drift_detector_initialized) return;

    // Positive drift = Video ahead of Audio; Negative drift = Video behind Audio
    int64_t drift = (int64_t)video_pts_us - (int64_t)audio_pts_us;
    g_current_drift_us = drift;

    int64_t abs_drift = (drift < 0) ? -drift : drift;
    if (abs_drift > g_max_drift_us) {
        g_max_drift_us = abs_drift;
    }

    g_avg_drift_us = (g_avg_drift_us == 0) ? drift : ((g_avg_drift_us * 7 + drift) / 8);
}

int64_t drift_detector_get_current_drift_us(void) {
    return g_current_drift_us;
}

int64_t drift_detector_get_avg_drift_us(void) {
    return g_avg_drift_us;
}

int64_t drift_detector_get_max_drift_us(void) {
    return g_max_drift_us;
}
