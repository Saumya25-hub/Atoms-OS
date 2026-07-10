/**
 * @file quality_engine.c
 * @brief AGDTE Phase 5 - Display Quality Engine Master Implementation
 * Coordinates motion continuity, frame stabilizer, dirty optimizer, and cadence optimizer.
 */

#include "quality_engine.h"
#include "frame_stabilizer.h"
#include "motion_analyzer.h"
#include "dirty_optimizer.h"
#include "cadence_optimizer.h"
#include "presentation_diag.h"
#include "display_metrics.h"
#include "present_quality.h"

static bool s_quality_engine_initialized = false;

void AGDTE_QualityEngine_Initialize(void) {
    if (s_quality_engine_initialized) {
        AGDTE_QualityEngine_Reset();
        return;
    }
    AGDTE_FrameStabilizer_Initialize();
    AGDTE_MotionAnalyzer_Initialize();
    AGDTE_DirtyOptimizer_Initialize();
    AGDTE_CadenceOptimizer_Initialize();
    AGDTE_PresentationDiag_Initialize();
    AGDTE_DisplayMetrics_Initialize();
    AGDTE_PresentQuality_Initialize();

    s_quality_engine_initialized = true;
}

void AGDTE_QualityEngine_Reset(void) {
    AGDTE_FrameStabilizer_Reset();
    AGDTE_MotionAnalyzer_Reset();
    AGDTE_DirtyOptimizer_Initialize();
    AGDTE_CadenceOptimizer_Reset();
    AGDTE_PresentationDiag_Reset();
    AGDTE_DisplayMetrics_Initialize();
}

bool AGDTE_QualityEngine_IsInitialized(void) {
    return s_quality_engine_initialized;
}

AGDTE_Error AGDTE_QualityEngine_ProcessFrame(AGDTE_PresentRequest* req, uint64_t current_time_us) {
    if (!s_quality_engine_initialized) {
        return AGDTE_OK; /* Transparent pass-through if uninitialized */
    }
    if (!req) {
        return AGDTE_ERR_NULL_POINTER;
    }

    uint32_t disp_id = req->display_id;

    /* 1. Cadence Alignment & Regularization */
    AGDTE_CadenceOptimizer_AlignRequest(req, current_time_us);

    /* 2. Burst & High-Frequency Stabilizer Check */
    AGDTE_Error stab_err = AGDTE_FrameStabilizer_StabilizeRequest(req, current_time_us);
    if (stab_err != AGDTE_OK) {
        AGDTE_PresentationDiag_RecordSkip(disp_id);
        return stab_err; /* Burst suppressed / duplicate skipped */
    }

    /* 3. Motion & Jitter Analysis */
    AGDTE_MotionAnalyzer_AnalyzeRequest(req, current_time_us);

    /* 4. Dirty Region Coalescing, Fragmentation Cap & VRAM Cache Alignment */
    AGDTE_DirtyOptimizer_OptimizeRequest(req);

    /* 5. Record Presentation Interval & Update Telemetry */
    uint64_t last_t = AGDTE_FrameStabilizer_GetLastPresentTime(disp_id);
    if (last_t > 0 && current_time_us > last_t) {
        uint64_t interval = current_time_us - last_t;
        AGDTE_PresentationDiag_RecordInterval(disp_id, (uint32_t)interval);
    }
    AGDTE_FrameStabilizer_RecordPresentation(disp_id, current_time_us);

    return AGDTE_OK;
}
