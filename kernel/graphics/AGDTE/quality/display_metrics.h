#ifndef ATOMS_OS_AGDTE_DISPLAY_METRICS_H
#define ATOMS_OS_AGDTE_DISPLAY_METRICS_H

/**
 * @file display_metrics.h
 * @brief AGDTE Phase 5 - Display Quality Metrics Module
 * Calculates composite integer quality scores (Pacing, Motion Continuity, Overall Display Quality).
 */

#include "../include/agdte.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t frame_pacing_score;       /* 0 to 100 */
    uint32_t motion_continuity_score;  /* 0 to 100 */
    uint32_t display_quality_score;    /* 0 to 100 composite */
    uint32_t current_fps_estimate;     /* Computed frames per second */
} AGDTE_DisplayQualityMetrics;

/**
 * @brief Initialize Display Metrics calculation module.
 */
void AGDTE_DisplayMetrics_Initialize(void);

/**
 * @brief Calculate and retrieve realtime quality metrics for a display.
 * @param display_id Target display ID.
 * @param out_metrics Output struct for calculated integer scores.
 */
void AGDTE_DisplayMetrics_Calculate(uint32_t display_id, AGDTE_DisplayQualityMetrics* out_metrics);

/**
 * @brief Get single composite display quality score (0 to 100).
 */
uint32_t AGDTE_DisplayMetrics_GetScore(uint32_t display_id);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_AGDTE_DISPLAY_METRICS_H */
