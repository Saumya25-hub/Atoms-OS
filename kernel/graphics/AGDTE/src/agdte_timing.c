/**
 * @file agdte_timing.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Deterministic Timing Engine
 * @status Phase 2 Core Architecture & Display Scheduler Frozen
 *
 * @section PURPOSE
 * Manages presentation timing, frame cadence calculations, deadline scheduling,
 * and deterministic synchronization intervals without floating point math or heap allocations.
 */

#include "../include/agdte.h"

/* --- Static Storage for Timing Records --- */
static AGDTE_TimingRecord s_timing_history[AGDTE_MAX_TIMING_HISTORY];
static uint32_t s_timing_head = 0;
static uint32_t s_timing_count = 0;
static uint64_t s_last_cadence_period_us = 16666; /* Default 60Hz = 16.666ms */

AGDTE_Error AGDTE_Timing_RecordSubmit(uint32_t request_id, uint64_t submit_time_us, uint64_t target_deadline_us) {
    if (s_timing_count < AGDTE_MAX_TIMING_HISTORY) {
        s_timing_count++;
    }
    
    AGDTE_TimingRecord* rec = &s_timing_history[s_timing_head];
    rec->frame_id = request_id;
    rec->submit_timestamp_us = submit_time_us;
    rec->scheduled_timestamp_us = 0;
    rec->presentation_timestamp_us = 0;
    rec->target_deadline_us = target_deadline_us;
    rec->cadence_period_us = (uint32_t)s_last_cadence_period_us;
    rec->vsync_aligned = false;

    s_timing_head = (s_timing_head + 1) % AGDTE_MAX_TIMING_HISTORY;
    return AGDTE_OK;
}

AGDTE_Error AGDTE_Timing_RecordPresentation(uint32_t request_id, uint64_t present_time_us) {
    for (uint32_t i = 0; i < s_timing_count; i++) {
        uint32_t idx = (s_timing_head + AGDTE_MAX_TIMING_HISTORY - 1 - i) % AGDTE_MAX_TIMING_HISTORY;
        if (s_timing_history[idx].frame_id == request_id) {
            s_timing_history[idx].presentation_timestamp_us = present_time_us;
            if (s_timing_history[idx].submit_timestamp_us > 0 && present_time_us >= s_timing_history[idx].submit_timestamp_us) {
                uint64_t latency = present_time_us - s_timing_history[idx].submit_timestamp_us;
                AGDTE_Diag_RecordLatency(latency);
            }
            return AGDTE_OK;
        }
    }
    return AGDTE_ERR_INVALID_STATE;
}

uint64_t AGDTE_Timing_CalculateCadenceDeadline(uint32_t display_id, uint64_t current_time_us) {
    AGDTE_DisplayState* disp = AGDTE_Display_GetState(display_id);
    if (!disp || !disp->active) {
        /* Default 60Hz cadence if display uninitialized */
        return current_time_us + 16666;
    }

    uint64_t period_us;
    switch (disp->cadence_mode) {
        case AGDTE_CADENCE_IMMEDIATE:
            return current_time_us; /* Immediate presentation deadline */
        case AGDTE_CADENCE_144HZ_FIXED:
            period_us = 6944; /* ~144Hz in microseconds */
            break;
        case AGDTE_CADENCE_ADAPTIVE_SYNC:
            /* Adaptive: allow early presentation if GPU/Display ready, else clamp to max refresh */
            period_us = (disp->refresh_rate_hz > 0) ? (1000000ULL / disp->refresh_rate_hz) : 16666;
            break;
        case AGDTE_CADENCE_VSYNC_IRQ:
            /* Align deadline to next VBI interval */
            if (disp->refresh_rate_hz > 0 && disp->last_vbi_timestamp_us > 0) {
                uint64_t interval_us = 1000000ULL / disp->refresh_rate_hz;
                uint64_t elapsed_since_vbi = current_time_us - disp->last_vbi_timestamp_us;
                uint64_t remainder = elapsed_since_vbi % interval_us;
                return current_time_us + (interval_us - remainder);
            }
            period_us = 16666;
            break;
        case AGDTE_CADENCE_60HZ_FIXED:
        default:
            period_us = 16666; /* ~60Hz in microseconds */
            break;
    }

    s_last_cadence_period_us = period_us;
    return current_time_us + period_us;
}
