#ifndef ATOMS_OS_AGDTE_PRESENT_QUALITY_H
#define ATOMS_OS_AGDTE_PRESENT_QUALITY_H

/**
 * @file present_quality.h
 * @brief AGDTE Phase 5 - Presentation Quality Management Header
 * Centralized quality telemetry dumping and inspection interface for system diagnostics HUD.
 */

#include "../include/agdte.h"
#include "display_metrics.h"
#include "presentation_diag.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Presentation Quality module.
 */
void AGDTE_PresentQuality_Initialize(void);

/**
 * @brief Perform comprehensive quality inspection and diagnostics log dump.
 */
void AGDTE_PresentQuality_DumpDiagnostics(uint32_t display_id);

/**
 * @brief Get overall visual smoothness indicator (true if score >= 90).
 */
bool AGDTE_PresentQuality_IsSmooth(uint32_t display_id);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_AGDTE_PRESENT_QUALITY_H */
