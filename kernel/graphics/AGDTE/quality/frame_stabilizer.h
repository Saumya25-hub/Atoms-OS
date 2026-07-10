#ifndef ATOMS_OS_AGDTE_FRAME_STABILIZER_H
#define ATOMS_OS_AGDTE_FRAME_STABILIZER_H

/**
 * @file frame_stabilizer.h
 * @brief AGDTE Phase 5 - Frame Stabilizer Module
 * Tracks presentation intervals and prevents burst presentation anomalies.
 */

#include "../include/agdte.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the Frame Stabilizer module.
 */
void AGDTE_FrameStabilizer_Initialize(void);

/**
 * @brief Reset Frame Stabilizer state across all displays.
 */
void AGDTE_FrameStabilizer_Reset(void);

/**
 * @brief Evaluate and stabilize a request before execution.
 * Checks for burst anomalies or uncoordinated high-frequency sub-frame swaps.
 *
 * @param req Pointer to presentation request.
 * @param current_time_us Current microsecond timestamp.
 * @return AGDTE_OK on clean stability, or error/skip code if stabilized/suppressed.
 */
AGDTE_Error AGDTE_FrameStabilizer_StabilizeRequest(AGDTE_PresentRequest* req, uint64_t current_time_us);

/**
 * @brief Record successful presentation timestamp to update interval history.
 * @param display_id Target display identifier.
 * @param present_time_us Exact presentation execution timestamp.
 */
void AGDTE_FrameStabilizer_RecordPresentation(uint32_t display_id, uint64_t present_time_us);

/**
 * @brief Get the last presentation timestamp for a display.
 */
uint64_t AGDTE_FrameStabilizer_GetLastPresentTime(uint32_t display_id);

/**
 * @brief Get the moving average presentation interval in microseconds for a display.
 */
uint32_t AGDTE_FrameStabilizer_GetAverageIntervalUs(uint32_t display_id);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_AGDTE_FRAME_STABILIZER_H */
