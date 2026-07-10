#ifndef ATOMS_OS_DISPLAY_DIAG_H
#define ATOMS_OS_DISPLAY_DIAG_H

/**
 * @file display_diag.h
 * @brief ATOMS OS Display Intelligence Engine - Official Diagnostics Header
 * Provides authoritative telemetry and inspection reporting across all display geometry,
 * policy decisions, detected hardware environments, and active capabilities.
 */

#include "display_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Print comprehensive diagnostic report to system console or serial log.
 */
void DIE_Diag_Dump(uint32_t display_id);

/**
 * @brief Write diagnostic summary string into provided buffer for HUD inspection.
 */
uint32_t DIE_Diag_GetSummaryString(uint32_t display_id, char* buffer, uint32_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_DISPLAY_DIAG_H */
