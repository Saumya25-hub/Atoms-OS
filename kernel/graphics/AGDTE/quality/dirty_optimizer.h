#ifndef ATOMS_OS_AGDTE_DIRTY_OPTIMIZER_H
#define ATOMS_OS_AGDTE_DIRTY_OPTIMIZER_H

/**
 * @file dirty_optimizer.h
 * @brief AGDTE Phase 5 - Dirty Region Optimizer Module
 * Coalesces fragmented rectangles, removes overlap, caps fragmentation, and enforces VRAM burst cache alignment.
 */

#include "../include/agdte.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the Dirty Optimizer module.
 */
void AGDTE_DirtyOptimizer_Initialize(void);

/**
 * @brief Optimize and coalesce the dirty regions of a presentation request.
 * Performs spatial proximity coalescing (`PROXIMITY_THRESHOLD_PX = 32`), caps fragmentation
 * (`dirty_count > 4` -> macroscopic bounding union), and cache-line aligns coordinates (`x & ~3`).
 *
 * @param req Pointer to presentation request to optimize in place.
 * @return AGDTE_OK on success, error code otherwise.
 */
AGDTE_Error AGDTE_DirtyOptimizer_OptimizeRequest(AGDTE_PresentRequest* req);

/**
 * @brief Get total count of dirty rectangles coalesced across all frames.
 */
uint32_t AGDTE_DirtyOptimizer_GetTotalCoalescedCount(void);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_AGDTE_DIRTY_OPTIMIZER_H */
