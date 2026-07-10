#ifndef ATOMS_OS_AGDTE_CADENCE_OPTIMIZER_H
#define ATOMS_OS_AGDTE_CADENCE_OPTIMIZER_H

/**
 * @file cadence_optimizer.h
 * @brief AGDTE Phase 5 - Cadence Optimizer Module
 * Regularizes display cadence intervals and prevents skipped cadence anomalies.
 */

#include "../include/agdte.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Cadence Optimizer states.
 */
void AGDTE_CadenceOptimizer_Initialize(void);

/**
 * @brief Reset Cadence Optimizer states across all displays.
 */
void AGDTE_CadenceOptimizer_Reset(void);

/**
 * @brief Regularize request deadline to exact display refresh cadence profile.
 *
 * @param req Pointer to presentation request.
 * @param current_time_us Current system microsecond timestamp.
 * @return AGDTE_OK on clean cadence alignment.
 */
AGDTE_Error AGDTE_CadenceOptimizer_AlignRequest(AGDTE_PresentRequest* req, uint64_t current_time_us);

/**
 * @brief Get the nominal target interval in microseconds for a display cadence profile.
 */
uint32_t AGDTE_CadenceOptimizer_GetTargetIntervalUs(uint32_t display_id);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_AGDTE_CADENCE_OPTIMIZER_H */
