#include "pointer_velocity.h"
#include "kernel/drivers/display/display.h"

#define VELOCITY_HISTORY_SIZE 4

typedef struct {
    int32_t dx;
    int32_t dy;
    uint64_t timestamp_us;
} VelocitySample;

static VelocitySample g_history[VELOCITY_HISTORY_SIZE];
static uint32_t g_history_idx = 0;
static PointerVelocityProfile g_profile;

static uint32_t int_sqrt(uint32_t n) {
    if (n == 0) return 0;
    uint32_t x = n;
    uint32_t y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

void pointer_velocity_init(void) {
    for (int i = 0; i < VELOCITY_HISTORY_SIZE; i++) {
        g_history[i].dx = 0;
        g_history[i].dy = 0;
        g_history[i].timestamp_us = 0;
    }
    g_history_idx = 0;

    // Calibrated Windows 11 / macOS 1080p Kinematic Ballistic Profile
    g_profile.base_sensitivity_fp16 = (int32_t)(1.35f * 65536.0f);       // 1.35x responsive baseline
    g_profile.velocity_threshold = 35;                                   // 35 px/sec inflection point
    g_profile.acceleration_gain_fp16 = (int32_t)(0.70f * 65536.0f);      // 0.70x smooth quadratic gain
    g_profile.max_sensitivity_fp16 = (int32_t)(3.80f * 65536.0f);        // 3.80x top speed clamp

    display_print("[POINTER VELOCITY] Configurable Kinematic Acceleration Initialized.\n");
}

void pointer_velocity_set_profile(const PointerVelocityProfile* profile) {
    if (profile) {
        g_profile = *profile;
    }
}

int32_t pointer_velocity_calculate(int32_t dx, int32_t dy, uint64_t timestamp_us, int32_t* out_velocity_raw) {
    uint32_t prev_idx = (g_history_idx - 1 + VELOCITY_HISTORY_SIZE) % VELOCITY_HISTORY_SIZE;
    uint64_t last_ts = g_history[prev_idx].timestamp_us;

    g_history[g_history_idx].dx = dx;
    g_history[g_history_idx].dy = dy;
    g_history[g_history_idx].timestamp_us = timestamp_us;
    g_history_idx = (g_history_idx + 1) % VELOCITY_HISTORY_SIZE;

    if (last_ts == 0 || timestamp_us <= last_ts || (timestamp_us - last_ts) > 500000) {
        if (out_velocity_raw) *out_velocity_raw = 0;
        return g_profile.base_sensitivity_fp16;
    }

    uint64_t dt_us = timestamp_us - last_ts;
    uint32_t dist_sq = (uint32_t)(dx * dx + dy * dy);
    uint32_t dist = int_sqrt(dist_sq);

    // Velocity in pixels per second
    uint32_t velocity_px_sec = (uint32_t)(((uint64_t)dist * 1000000ULL) / dt_us);
    if (out_velocity_raw) {
        *out_velocity_raw = (int32_t)velocity_px_sec;
    }

    if (velocity_px_sec <= (uint32_t)g_profile.velocity_threshold || g_profile.velocity_threshold == 0) {
        return g_profile.base_sensitivity_fp16;
    }

    uint32_t extra_v = velocity_px_sec - (uint32_t)g_profile.velocity_threshold;
    int64_t gain_contribution = ((int64_t)extra_v * (int64_t)g_profile.acceleration_gain_fp16) / g_profile.velocity_threshold;
    int64_t accel_fp16 = (int64_t)g_profile.base_sensitivity_fp16 + gain_contribution;

    if (accel_fp16 > (int64_t)g_profile.max_sensitivity_fp16) {
        accel_fp16 = g_profile.max_sensitivity_fp16;
    }

    return (int32_t)accel_fp16;
}
