/**
 * @file pointer_predict.c
 * @brief Kinematic Subpixel Trajectory Prediction Engine V3.0 Implementation
 * @section PURPOSE
 * Implements 2nd-order Taylor expansion trajectory prediction:
 * P_pred = P_curr + V * dT + 0.5 * A * dT^2
 * Eliminates hand-to-screen scanout latency for the signature "Floating-on-Glass" feel.
 */

#include "pointer_predict.h"
#include "kernel/drivers/display/display.h"

typedef struct {
    int32_t last_dx;
    int32_t last_dy;
    int32_t last_vx; // 16.16 fixed-point
    int32_t last_vy; // 16.16 fixed-point
    uint64_t last_ts_us;
} PredictState;

static PredictState g_pred_state = {0};

void pointer_predict_init(void) {
    g_pred_state.last_dx = 0;
    g_pred_state.last_dy = 0;
    g_pred_state.last_vx = 0;
    g_pred_state.last_vy = 0;
    g_pred_state.last_ts_us = 0;
    display_print("[POINTER PREDICT] Windows NT Kinematic Trajectory Prediction Initialized.\n");
}

void pointer_predict_apply(int32_t current_dx, int32_t current_dy,
                           uint64_t current_ts_us, uint32_t target_scanout_delta_us,
                           int32_t* out_pred_dx, int32_t* out_pred_dy) {
    if (!out_pred_dx || !out_pred_dy) return;

    if (g_pred_state.last_ts_us == 0 || current_ts_us <= g_pred_state.last_ts_us) {
        g_pred_state.last_dx = current_dx;
        g_pred_state.last_dy = current_dy;
        g_pred_state.last_ts_us = current_ts_us;
        *out_pred_dx = current_dx;
        *out_pred_dy = current_dy;
        return;
    }

    uint64_t dt_us = current_ts_us - g_pred_state.last_ts_us;
    if (dt_us > 100000) { // Timeout > 100ms: reset velocity history
        g_pred_state.last_vx = 0;
        g_pred_state.last_vy = 0;
        g_pred_state.last_ts_us = current_ts_us;
        *out_pred_dx = current_dx;
        *out_pred_dy = current_dy;
        return;
    }

    /* Compute velocity in 16.16 fixed-point pixels per microsecond */
    int64_t curr_vx = ((int64_t)current_dx << 16) / (int64_t)dt_us;
    int64_t curr_vy = ((int64_t)current_dy << 16) / (int64_t)dt_us;

    /* Compute acceleration vector A = (V_curr - V_prev) / dt */
    int64_t ax = ((curr_vx - g_pred_state.last_vx) << 16) / (int64_t)dt_us;
    int64_t ay = ((curr_vy - g_pred_state.last_vy) << 16) / (int64_t)dt_us;

    /* Default scanout lead time if 0 specified: 8333us (8.33ms = 1/120s lead) */
    uint64_t dT = (target_scanout_delta_us > 0) ? target_scanout_delta_us : 8333U;

    /* 2nd-Order Taylor Expansion: P_pred = P_curr + V * dT + 0.5 * A * dT^2 */
    int64_t pred_offset_x = (curr_vx * (int64_t)dT) + ((ax * (int64_t)dT * (int64_t)dT) >> 17);
    int64_t pred_offset_y = (curr_vy * (int64_t)dT) + ((ay * (int64_t)dT * (int64_t)dT) >> 17);

    int32_t predicted_dx = current_dx + (int32_t)(pred_offset_x >> 16);
    int32_t predicted_dy = current_dy + (int32_t)(pred_offset_y >> 16);

    /* Clamp maximum prediction offset to avoid overshooting (Max 12 pixels lead) */
    if (predicted_dx > current_dx + 12) predicted_dx = current_dx + 12;
    if (predicted_dx < current_dx - 12) predicted_dx = current_dx - 12;
    if (predicted_dy > current_dy + 12) predicted_dy = current_dy + 12;
    if (predicted_dy < current_dy - 12) predicted_dy = current_dy - 12;

    *out_pred_dx = predicted_dx;
    *out_pred_dy = predicted_dy;

    /* Update history state */
    g_pred_state.last_dx = current_dx;
    g_pred_state.last_dy = current_dy;
    g_pred_state.last_vx = (int32_t)curr_vx;
    g_pred_state.last_vy = (int32_t)curr_vy;
    g_pred_state.last_ts_us = current_ts_us;
}
