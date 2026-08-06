/**
 * @file pointer_predict.h
 * @brief Kinematic Subpixel Trajectory Prediction Engine V3.0
 * @section PURPOSE
 * Calculates trajectory prediction vector P_pred = P_curr + V*dT + 0.5*A*dT^2
 * matching Windows NT (win32k.sys) and macOS Quartz to eliminate physical hand-lag.
 */

#ifndef KERNEL_POINTER_PREDICT_H
#define KERNEL_POINTER_PREDICT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void pointer_predict_init(void);

/**
 * Predict cursor position at scanout time (8.3ms - 16.6ms ahead)
 */
void pointer_predict_apply(int32_t current_dx, int32_t current_dy,
                           uint64_t current_ts_us, uint32_t target_scanout_delta_us,
                           int32_t* out_pred_dx, int32_t* out_pred_dy);

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_POINTER_PREDICT_H */
