/**
 * @file presentation_diag.c
 * @brief AGDTE Phase 5 - Presentation Diagnostics Implementation
 */

#include "presentation_diag.h"

static AGDTE_QualityDiagTelemetry s_quality_diag[AGDTE_MAX_DISPLAYS];

void AGDTE_PresentationDiag_Initialize(void) {
    AGDTE_PresentationDiag_Reset();
}

void AGDTE_PresentationDiag_Reset(void) {
    for (uint32_t i = 0; i < AGDTE_MAX_DISPLAYS; i++) {
        s_quality_diag[i].total_frames_polished = 0;
        s_quality_diag[i].total_dirty_merges = 0;
        s_quality_diag[i].total_bursts_suppressed = 0;
        s_quality_diag[i].total_skips_recorded = 0;
        s_quality_diag[i].average_frame_interval_us = 16666;
        s_quality_diag[i].worst_frame_interval_us = 16666;
        s_quality_diag[i].presentation_variance_us = 0;
    }
}

void AGDTE_PresentationDiag_RecordDirtyMerge(uint32_t display_id) {
    if (display_id < AGDTE_MAX_DISPLAYS) {
        s_quality_diag[display_id].total_dirty_merges++;
    }
}

void AGDTE_PresentationDiag_RecordBurst(uint32_t display_id) {
    if (display_id < AGDTE_MAX_DISPLAYS) {
        s_quality_diag[display_id].total_bursts_suppressed++;
    }
}

void AGDTE_PresentationDiag_RecordSkip(uint32_t display_id) {
    if (display_id < AGDTE_MAX_DISPLAYS) {
        s_quality_diag[display_id].total_skips_recorded++;
    }
}

void AGDTE_PresentationDiag_RecordVariance(uint32_t display_id, uint32_t variance_us) {
    if (display_id < AGDTE_MAX_DISPLAYS) {
        s_quality_diag[display_id].presentation_variance_us = variance_us;
    }
}

void AGDTE_PresentationDiag_RecordInterval(uint32_t display_id, uint32_t interval_us) {
    if (display_id < AGDTE_MAX_DISPLAYS) {
        s_quality_diag[display_id].total_frames_polished++;
        s_quality_diag[display_id].average_frame_interval_us = 
            (s_quality_diag[display_id].average_frame_interval_us * 7 + interval_us) >> 3;
        if (interval_us > s_quality_diag[display_id].worst_frame_interval_us) {
            s_quality_diag[display_id].worst_frame_interval_us = interval_us;
        }
    }
}

void AGDTE_PresentationDiag_GetTelemetry(uint32_t display_id, AGDTE_QualityDiagTelemetry* out_telemetry) {
    if (display_id < AGDTE_MAX_DISPLAYS && out_telemetry) {
        *out_telemetry = s_quality_diag[display_id];
    }
}
