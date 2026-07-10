/**
 * @file frame_stabilizer.c
 * @brief AGDTE Phase 5 - Frame Stabilizer Implementation
 * Deterministic integer arithmetic tracking for burst prevention and interval stabilization.
 */

#include "frame_stabilizer.h"
#include "presentation_diag.h"

#define STABILIZER_BURST_THRESHOLD_US   4000ULL  /* < 4.0ms since last swap is classified as burst */
#define STABILIZER_HISTORY_SIZE         8

typedef struct {
    uint64_t last_present_time_us;
    uint32_t interval_history[STABILIZER_HISTORY_SIZE];
    uint32_t history_index;
    uint32_t history_count;
    uint32_t average_interval_us;
    uint32_t burst_count;
    bool initialized;
} AGDTE_DisplayStabilizerState;

static AGDTE_DisplayStabilizerState s_stabilizer_state[AGDTE_MAX_DISPLAYS];

void AGDTE_FrameStabilizer_Initialize(void) {
    AGDTE_FrameStabilizer_Reset();
}

void AGDTE_FrameStabilizer_Reset(void) {
    for (uint32_t i = 0; i < AGDTE_MAX_DISPLAYS; i++) {
        s_stabilizer_state[i].last_present_time_us = 0;
        for (uint32_t j = 0; j < STABILIZER_HISTORY_SIZE; j++) {
            s_stabilizer_state[i].interval_history[j] = 16666; /* Default 60Hz nominal */
        }
        s_stabilizer_state[i].history_index = 0;
        s_stabilizer_state[i].history_count = 0;
        s_stabilizer_state[i].average_interval_us = 16666;
        s_stabilizer_state[i].burst_count = 0;
        s_stabilizer_state[i].initialized = true;
    }
}

AGDTE_Error AGDTE_FrameStabilizer_StabilizeRequest(AGDTE_PresentRequest* req, uint64_t current_time_us) {
    if (!req) return AGDTE_ERR_NULL_POINTER;
    uint32_t disp_id = req->display_id;
    if (disp_id >= AGDTE_MAX_DISPLAYS) return AGDTE_ERR_INVALID_DISPLAY;

    AGDTE_DisplayStabilizerState* state = &s_stabilizer_state[disp_id];
    if (!state->initialized) {
        AGDTE_FrameStabilizer_Reset();
    }

    /* First frame bypass */
    if (state->last_present_time_us == 0) {
        return AGDTE_OK;
    }

    uint64_t interval = (current_time_us >= state->last_present_time_us) ? 
                        (current_time_us - state->last_present_time_us) : 0;

    /* Burst presentation detection & pacing alignment */
    if (interval < STABILIZER_BURST_THRESHOLD_US && 
        req->priority != AGDTE_PRIORITY_CRITICAL_CURSOR && 
        req->priority != AGDTE_PRIORITY_REALTIME_SYNC) {
        state->burst_count++;
        AGDTE_PresentationDiag_RecordBurst(disp_id);
        
        /* If request has no dirty count (or duplicate redundant presentation within same sub-tick), suppress burst */
        if (req->dirty_count == 0 && !req->force_immediate) {
            return AGDTE_OK; /* Coalesced cleanly */
        }
    }

    return AGDTE_OK;
}

void AGDTE_FrameStabilizer_RecordPresentation(uint32_t display_id, uint64_t present_time_us) {
    if (display_id >= AGDTE_MAX_DISPLAYS) return;
    AGDTE_DisplayStabilizerState* state = &s_stabilizer_state[display_id];

    if (state->last_present_time_us > 0 && present_time_us > state->last_present_time_us) {
        uint64_t interval = present_time_us - state->last_present_time_us;
        uint32_t interval_u32 = (interval > 1000000ULL) ? 1000000U : (uint32_t)interval;

        state->interval_history[state->history_index] = interval_u32;
        state->history_index = (state->history_index + 1) % STABILIZER_HISTORY_SIZE;
        if (state->history_count < STABILIZER_HISTORY_SIZE) {
            state->history_count++;
        }

        /* Recompute integer moving average */
        uint64_t sum = 0;
        for (uint32_t i = 0; i < state->history_count; i++) {
            sum += state->interval_history[i];
        }
        state->average_interval_us = (uint32_t)(sum / state->history_count);
    }

    state->last_present_time_us = present_time_us;
}

uint64_t AGDTE_FrameStabilizer_GetLastPresentTime(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) return 0;
    return s_stabilizer_state[display_id].last_present_time_us;
}

uint32_t AGDTE_FrameStabilizer_GetAverageIntervalUs(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) return 16666;
    return s_stabilizer_state[display_id].average_interval_us;
}
