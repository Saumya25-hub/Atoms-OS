#ifndef ATOMS_OS_BSPE_PRESENT_H
#define ATOMS_OS_BSPE_PRESENT_H

/**
 * @file bspe_present.h
 * @brief BSPE Presentation Engine & Integration Layer (SwapFull Adapter)
 * @status Step 10 Production Implementation
 * 
 * @section PURPOSE
 * Internal header for the BSPE Presentation Engine integration layer.
 * Declares the legacy SwapFull backend bridge and internal telemetry structures.
 * 
 * @section DEPENDENCY_LIST
 * - "../include/bspe.h"
 * - Zero circular includes.
 */

#include "../include/bspe.h"
#include "../Debug/telemetry_hud.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Internal legacy backend bridge executing the original full-frame VRAM copy.
 * @param hw_fb Pointer to destination hardware framebuffer (BVFramebuffer*).
 */
void BOVISUAL_Graphics_LegacySwapFull_Backend(const void* hw_fb);

/**
 * @brief Retrieves current internal telemetry metrics of the BSPE engine.
 * @param out_metrics Pointer to caller-owned telemetry structure to populate.
 * @return BSPE_OK on success; BSPE_ERR_NULL_POINTER if out_metrics is NULL.
 */
BSPE_Error BSPE_GetTelemetryMetrics(BSPE_TelemetryMetrics* out_metrics);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BSPE_PRESENT_H
