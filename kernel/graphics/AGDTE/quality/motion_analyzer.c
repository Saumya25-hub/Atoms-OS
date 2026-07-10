/**
 * @file motion_analyzer.c
 * @brief AGDTE Phase 5 - Motion Analyzer Implementation
 * Deterministic fixed-point integer math for tracking motion variance and frame interval jitter.
 */

#include "motion_analyzer.h"
#include "presentation_diag.h"

#define MOTION_HISTORY_SLOTS    16

typedef struct {
    int32_t last_cursor_x;
    int32_t last_cursor_y;
    uint64_t last_cursor_time_us;
    
    uint64_t last_request_time_us;
    uint32_t interval_history[MOTION_HISTORY_SLOTS];
    uint32_t history_idx;
    uint32_t history_cnt;
    
    uint32_t current_jitter_us;
    uint32_t consistency_score;
    bool initialized;
} AGDTE_MotionState;

static AGDTE_MotionState s_motion_state[AGDTE_MAX_DISPLAYS];

void AGDTE_MotionAnalyzer_Initialize(void) {
    AGDTE_MotionAnalyzer_Reset();
}

void AGDTE_MotionAnalyzer_Reset(void) {
    for (uint32_t i = 0; i < AGDTE_MAX_DISPLAYS; i++) {
        s_motion_state[i].last_cursor_x = 0;
        s_motion_state[i].last_cursor_y = 0;
        s_motion_state[i].last_cursor_time_us = 0;
        s_motion_state[i].last_request_time_us = 0;
        for (uint32_t j = 0; j < MOTION_HISTORY_SLOTS; j++) {
            s_motion_state[i].interval_history[j] = 16666;
        }
        s_motion_state[i].history_idx = 0;
        s_motion_state[i].history_cnt = 0;
        s_motion_state[i].current_jitter_us = 0;
        s_motion_state[i].consistency_score = 100;
        s_motion_state[i].initialized = true;
    }
}

void AGDTE_MotionAnalyzer_ObserveCursor(int32_t x, int32_t y, uint64_t timestamp_us) {
    /* Passive observation of cursor movement delta */
    AGDTE_MotionState* state = &s_motion_state[0]; /* Primary display observation */
    if (!state->initialized) {
        AGDTE_MotionAnalyzer_Reset();
    }
    state->last_cursor_x = x;
    state->last_cursor_y = y;
    state->last_cursor_time_us = timestamp_us;
}

void AGDTE_MotionAnalyzer_ObserveSurface(uint32_t surface_id, BOGE_Rect bounds, uint64_t timestamp_us) {
    (void)surface_id;
    (void)bounds;
    (void)timestamp_us;
    /* Passive observation hook for surface movement trajectory */
}

void AGDTE_MotionAnalyzer_AnalyzeRequest(const AGDTE_PresentRequest* req, uint64_t current_time_us) {
    if (!req) return;
    uint32_t disp_id = req->display_id;
    if (disp_id >= AGDTE_MAX_DISPLAYS) return;

    AGDTE_MotionState* state = &s_motion_state[disp_id];
    if (!state->initialized) {
        AGDTE_MotionAnalyzer_Reset();
    }

    if (state->last_request_time_us > 0 && current_time_us > state->last_request_time_us) {
        uint64_t interval = current_time_us - state->last_request_time_us;
        uint32_t dt_us = (interval > 500000ULL) ? 500000U : (uint32_t)interval;

        state->interval_history[state->history_idx] = dt_us;
        state->history_idx = (state->history_idx + 1) % MOTION_HISTORY_SLOTS;
        if (state->history_cnt < MOTION_HISTORY_SLOTS) {
            state->history_cnt++;
        }

        /* Calculate mean interval */
        uint64_t sum = 0;
        for (uint32_t i = 0; i < state->history_cnt; i++) {
            sum += state->interval_history[i];
        }
        uint32_t mean = (uint32_t)(sum / state->history_cnt);

        /* Calculate absolute mean deviation (Jitter in us) using pure integer math */
        uint64_t dev_sum = 0;
        for (uint32_t i = 0; i < state->history_cnt; i++) {
            uint32_t val = state->interval_history[i];
            uint32_t diff = (val > mean) ? (val - mean) : (mean - val);
            dev_sum += diff;
        }
        state->current_jitter_us = (uint32_t)(dev_sum / state->history_cnt);

        /* Compute Consistency Score (out of 100): every 250 us of jitter docks 1 point */
        uint32_t penalty = state->current_jitter_us / 250;
        state->consistency_score = (penalty >= 100) ? 0 : (100 - penalty);

        AGDTE_PresentationDiag_RecordVariance(disp_id, state->current_jitter_us);
    }

    state->last_request_time_us = current_time_us;
}

uint32_t AGDTE_MotionAnalyzer_GetJitterUs(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) return 0;
    return s_motion_state[display_id].current_jitter_us;
}

uint32_t AGDTE_MotionAnalyzer_GetConsistencyScore(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) return 100;
    return s_motion_state[display_id].consistency_score;
}
