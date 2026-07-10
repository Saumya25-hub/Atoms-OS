/**
 * @file cadence_optimizer.c
 * @brief AGDTE Phase 5 - Cadence Optimizer Implementation
 * Regularizes display interval steps using deterministic integer calculations.
 */

#include "cadence_optimizer.h"

typedef struct {
    uint32_t target_interval_us;
    uint64_t last_cadence_time_us;
    uint32_t phase_offset_us;
    bool initialized;
} AGDTE_CadenceState;

static AGDTE_CadenceState s_cadence_state[AGDTE_MAX_DISPLAYS];

void AGDTE_CadenceOptimizer_Initialize(void) {
    AGDTE_CadenceOptimizer_Reset();
}

void AGDTE_CadenceOptimizer_Reset(void) {
    for (uint32_t i = 0; i < AGDTE_MAX_DISPLAYS; i++) {
        s_cadence_state[i].target_interval_us = 16666; /* Default 60Hz */
        s_cadence_state[i].last_cadence_time_us = 0;
        s_cadence_state[i].phase_offset_us = 0;
        s_cadence_state[i].initialized = true;
    }
}

AGDTE_Error AGDTE_CadenceOptimizer_AlignRequest(AGDTE_PresentRequest* req, uint64_t current_time_us) {
    if (!req) return AGDTE_ERR_NULL_POINTER;
    uint32_t disp_id = req->display_id;
    if (disp_id >= AGDTE_MAX_DISPLAYS) return AGDTE_ERR_INVALID_DISPLAY;

    AGDTE_CadenceState* state = &s_cadence_state[disp_id];
    if (!state->initialized) {
        AGDTE_CadenceOptimizer_Reset();
    }

    /* Dynamically adjust nominal interval if refresh rate is known */
    AGDTE_DisplayState* disp = AGDTE_Display_GetState(disp_id);
    if (disp && disp->refresh_rate_hz > 0) {
        state->target_interval_us = 1000000U / disp->refresh_rate_hz;
    }

    /* First frame initialization */
    if (state->last_cadence_time_us == 0) {
        state->last_cadence_time_us = current_time_us;
        return AGDTE_OK;
    }

    /* Calculate phase alignment target */
    uint64_t expected_target = state->last_cadence_time_us + state->target_interval_us;
    if (req->target_deadline_us < expected_target && 
        req->priority != AGDTE_PRIORITY_CRITICAL_CURSOR && 
        req->priority != AGDTE_PRIORITY_REALTIME_SYNC) {
        /* Align deadline to avoid premature interval clipping */
        req->target_deadline_us = expected_target;
    }

    state->last_cadence_time_us = req->target_deadline_us;
    return AGDTE_OK;
}

uint32_t AGDTE_CadenceOptimizer_GetTargetIntervalUs(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) return 16666;
    return s_cadence_state[display_id].target_interval_us;
}
