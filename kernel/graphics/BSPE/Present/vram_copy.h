#ifndef ATOMS_OS_BSPE_VRAM_COPY_H
#define ATOMS_OS_BSPE_VRAM_COPY_H

/**
 * @file vram_copy.h
 * @brief BSPE Partial VRAM Copy Engine Public Header
 * @status Step 11 Production Implementation
 * 
 * @section PURPOSE
 * Defines the public API, runtime feature flag, and telemetry structures for the
 * BSPE Partial VRAM Copy Engine. Copies only damaged rectangular scanlines to VRAM
 * without modifying source framebuffers or performing heap allocations.
 * 
 * @section DEPENDENCY_LIST
 * - "../include/bspe.h"
 * - Zero circular includes.
 */

#include "../include/bspe.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Required runtime flag controlling presentation mode.
 * false = Legacy Full Copy (Emergency fallback / Default)
 * true  = Partial Damage Copy
 */
extern bool bspe_use_partial_present;

/**
 * @brief Telemetry statistics structure for VRAM presentation copies.
 */
typedef struct {
    uint64_t full_copy_count;
    uint64_t partial_copy_count;
    uint64_t total_bytes_copied;
    uint64_t total_rects_copied;
    uint32_t average_bytes_per_frame;
    uint32_t largest_rect_area;
    uint32_t smallest_rect_area;
} BSPE_CopyTelemetry;

/**
 * @brief Copies damaged rectangular regions from system RAM staging framebuffer to VRAM.
 * 
 * Performs bounding box clipping against screen dimensions, skips offscreen/empty rects,
 * and executes fast row-by-row 64-bit SIMD-style block transfers.
 * If damage_count == 0 or bspe_use_partial_present == false, automatically falls back
 * to executing LegacySwapFullBackend().
 * 
 * @param frame Pointer to caller-owned staging frame containing buffer and damage list.
 * @return BSPE_OK on success; error code on invalid parameters.
 */
BSPE_Error BSPE_VRAM_CopyDamaged(const BOGE_StagingFrame* frame);

/**
 * @brief Copies an explicitly evaluated effective damage rectangle list to VRAM.
 * Step 12 Dual-Page Damage Presentation Entry Point.
 */
BSPE_Error BSPE_VRAM_CopyEffectiveDamage(const BOGE_StagingFrame* frame, const BOGE_Rect* effective_rects, uint32_t effective_count);

/**
 * @brief Retrieves current copy telemetry metrics.
 * @param out_telemetry Pointer to caller-owned structure to populate.
 */
void BSPE_VRAM_GetCopyTelemetry(BSPE_CopyTelemetry* out_telemetry);

/**
 * @brief Resets copy telemetry metrics.
 */
void BSPE_VRAM_ResetCopyTelemetry(void);

/**
 * @brief Executes comprehensive verification stress tests (1 rect, 10 rects, 32 rects,
 * overlapping, clipping, fullscreen, empty, random stress) and compares against Legacy SwapFull.
 * @return true if all tests pass with 100% pixel identity; false otherwise.
 */
bool BSPE_VRAM_RunStressTest(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BSPE_VRAM_COPY_H
