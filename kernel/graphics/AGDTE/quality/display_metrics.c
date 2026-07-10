/**
 * @file display_metrics.c
 * @brief AGDTE Phase 5 - Display Quality Metrics Implementation
 * Deterministic integer composite quality scoring (0..100).
 */

#include "display_metrics.h"
#include "frame_stabilizer.h"
#include "motion_analyzer.h"
#include "presentation_diag.h"

static AGDTE_DisplayQualityMetrics s_metrics[AGDTE_MAX_DISPLAYS];

void AGDTE_DisplayMetrics_Initialize(void) {
    for (uint32_t i = 0; i < AGDTE_MAX_DISPLAYS; i++) {
        s_metrics[i].frame_pacing_score = 100;
        s_metrics[i].motion_continuity_score = 100;
        s_metrics[i].display_quality_score = 100;
        s_metrics[i].current_fps_estimate = 60;
    }
}

void AGDTE_DisplayMetrics_Calculate(uint32_t display_id, AGDTE_DisplayQualityMetrics* out_metrics) {
    if (display_id >= AGDTE_MAX_DISPLAYS || !out_metrics) return;

    /* 1. Frame Pacing Score based on jitter and variance */
    uint32_t jitter = AGDTE_MotionAnalyzer_GetJitterUs(display_id);
    uint32_t pacing_penalty = jitter / 200; /* Every 200 us of jitter docks 1 point */
    s_metrics[display_id].frame_pacing_score = (pacing_penalty >= 100) ? 0 : (100 - pacing_penalty);

    /* 2. Motion Continuity Score */
    s_metrics[display_id].motion_continuity_score = AGDTE_MotionAnalyzer_GetConsistencyScore(display_id);

    /* 3. Composite Display Quality Score: 60% Pacing + 40% Motion Continuity */
    s_metrics[display_id].display_quality_score = 
        ((s_metrics[display_id].frame_pacing_score * 60) + 
         (s_metrics[display_id].motion_continuity_score * 40)) / 100;

    /* 4. Realtime FPS Estimate from average interval */
    uint32_t avg_interval = AGDTE_FrameStabilizer_GetAverageIntervalUs(display_id);
    if (avg_interval > 0) {
        s_metrics[display_id].current_fps_estimate = 1000000U / avg_interval;
    } else {
        s_metrics[display_id].current_fps_estimate = 60;
    }

    *out_metrics = s_metrics[display_id];
}

uint32_t AGDTE_DisplayMetrics_GetScore(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) return 100;
    AGDTE_DisplayQualityMetrics m;
    AGDTE_DisplayMetrics_Calculate(display_id, &m);
    return m.display_quality_score;
}
