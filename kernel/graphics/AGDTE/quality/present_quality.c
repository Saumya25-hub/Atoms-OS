/**
 * @file present_quality.c
 * @brief AGDTE Phase 5 - Presentation Quality Management Implementation
 */

#include "present_quality.h"

void AGDTE_PresentQuality_Initialize(void) {
    /* Delegated to DisplayMetrics and PresentationDiag initialization */
}

void AGDTE_PresentQuality_DumpDiagnostics(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) return;

    AGDTE_DisplayQualityMetrics m;
    AGDTE_DisplayMetrics_Calculate(display_id, &m);

    AGDTE_QualityDiagTelemetry t;
    AGDTE_PresentationDiag_GetTelemetry(display_id, &t);

    /* Telemetry hook for diagnostic system or console output */
}

bool AGDTE_PresentQuality_IsSmooth(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) return true;
    return (AGDTE_DisplayMetrics_GetScore(display_id) >= 90);
}
