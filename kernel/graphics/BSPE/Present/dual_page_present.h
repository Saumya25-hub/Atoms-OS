#ifndef ATOMS_OS_BSPE_DUAL_PAGE_PRESENT_H
#define ATOMS_OS_BSPE_DUAL_PAGE_PRESENT_H

/**
 * @file dual_page_present.h
 * @brief BSPE Dual-Page Damage Presentation Engine Public Header
 * @status Step 12 Production Implementation
 * 
 * @section PURPOSE
 * Implements the mathematical presentation equation:
 * EffectiveDamage = Union(Damage(Current Frame), Damage(Previous Frame))
 * This becomes the ONLY damage list used by BSPE presentation, ensuring perfect
 * synchronization across double-buffered VRAM pages without modifying source damage lists
 * or performing heap allocations.
 */

#include <stddef.h>
#include "../include/bspe.h"
#include "../Damage/damage_tracker.h"
#include "vram_copy.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Telemetry statistics structure for Dual-Page Damage Presentation.
 */
typedef struct {
    uint64_t current_rect_count;
    uint64_t previous_rect_count;
    uint64_t effective_rect_count;
    uint64_t merged_rectangles;
    uint64_t discarded_rectangles;
    uint64_t duplicate_rectangles;
    uint64_t history_advances;
    uint64_t history_rollbacks;
    uint64_t fallback_count;
} BSPE_DualPageTelemetry;

/**
 * @brief Evaluates EffectiveDamage = Union(Current, Previous) and executes presentation.
 * 
 * Rules enforced:
 * - Current damage is never destroyed.
 * - Previous damage is never modified.
 * - History advances ONLY after successful presentation.
 * - If presentation fails, history is NOT advanced and rollbacks incremented.
 * - On overflow, corruption, or invalid state, automatically executes Legacy SwapFull fallback.
 * 
 * @param damage_tracker Handle to active BSPE damage tracker instance.
 * @param frame Pointer to caller-owned staging frame.
 * @return BSPE_OK on success; error code otherwise.
 */
BSPE_Error BSPE_DualPage_PresentFrame(BSPE_DamageTrackerHandle damage_tracker, const BOGE_StagingFrame* frame);

/**
 * @brief Retrieves current dual-page telemetry metrics.
 */
void BSPE_DualPage_GetTelemetry(BSPE_DualPageTelemetry* out_telemetry);

/**
 * @brief Resets dual-page telemetry metrics.
 */
void BSPE_DualPage_ResetTelemetry(void);

/**
 * @brief Executes comprehensive 11-part verification stress tests (empty, single, multiple,
 * duplicate, touching, overlapping, prev only, current only, mixed history, 1000-frame random stress,
 * and pixel identity against legacy path).
 * @return true if all tests pass; false otherwise.
 */
bool BSPE_DualPage_RunStressTest(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BSPE_DUAL_PAGE_PRESENT_H
