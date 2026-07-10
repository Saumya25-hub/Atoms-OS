#ifndef ATOMS_OS_AGDTE_PRESENTATION_DIAG_H
#define ATOMS_OS_AGDTE_PRESENTATION_DIAG_H

/**
 * @file presentation_diag.h
 * @brief AGDTE Phase 5 - Presentation Quality Diagnostics & Telemetry Header
 * Records realtime presentation interval metrics, variance, bursts, merges, and skips.
 */

#include "../include/agdte.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t total_frames_polished;
    uint32_t total_dirty_merges;
    uint32_t total_bursts_suppressed;
    uint32_t total_skips_recorded;
    uint32_t average_frame_interval_us;
    uint32_t worst_frame_interval_us;
    uint32_t presentation_variance_us;
} AGDTE_QualityDiagTelemetry;

/**
 * @brief Initialize Presentation Diagnostics.
 */
void AGDTE_PresentationDiag_Initialize(void);

/**
 * @brief Reset all diagnostics telemetry.
 */
void AGDTE_PresentationDiag_Reset(void);

/**
 * @brief Record a dirty region merge event.
 */
void AGDTE_PresentationDiag_RecordDirtyMerge(uint32_t display_id);

/**
 * @brief Record a presentation burst event.
 */
void AGDTE_PresentationDiag_RecordBurst(uint32_t display_id);

/**
 * @brief Record a presentation skip event.
 */
void AGDTE_PresentationDiag_RecordSkip(uint32_t display_id);

/**
 * @brief Record presentation interval variance.
 */
void AGDTE_PresentationDiag_RecordVariance(uint32_t display_id, uint32_t variance_us);

/**
 * @brief Record presentation interval observation.
 */
void AGDTE_PresentationDiag_RecordInterval(uint32_t display_id, uint32_t interval_us);

/**
 * @brief Retrieve snapshot of current quality telemetry.
 */
void AGDTE_PresentationDiag_GetTelemetry(uint32_t display_id, AGDTE_QualityDiagTelemetry* out_telemetry);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_AGDTE_PRESENTATION_DIAG_H */
