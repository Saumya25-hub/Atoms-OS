/**
 * @file agdte_frame_pacer.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Deterministic Frame Pacer
 * @status Phase 4 Display Timing Optimization Layer
 *
 * @section PURPOSE
 * Enforces rigid, microsecond-accurate frame cadence intervals (`target_interval_us`)
 * across arbitrary refresh profiles (60Hz, 75Hz, 90Hz, 120Hz, 144Hz, 240Hz). Prevents
 * presentation burst, starvation, and quantization jitter without floating point math.
 */

#include "../include/agdte.h"

static AGDTE_FramePacerState s_pacers[AGDTE_MAX_DISPLAYS];

AGDTE_Error AGDTE_Pacer_Init(uint32_t display_id, uint32_t target_refresh_hz) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    AGDTE_FramePacerState* fp = &s_pacers[display_id];
    fp->display_id = display_id;
    fp->active = true;
    fp->last_presentation_us = 0;
    fp->next_presentation_deadline_us = 0;
    fp->jitter_tolerance_us = 1000; /* +/- 1ms window for alignment */

    return AGDTE_Pacer_SetRate(display_id, target_refresh_hz);
}

AGDTE_Error AGDTE_Pacer_SetRate(uint32_t display_id, uint32_t target_refresh_hz) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    AGDTE_FramePacerState* fp = &s_pacers[display_id];
    uint32_t hz = (target_refresh_hz > 0) ? target_refresh_hz : 60;
    fp->target_rate_hz = hz;

    switch (hz) {
        case 60:  fp->target_interval_us = 16666; break;
        case 75:  fp->target_interval_us = 13333; break;
        case 90:  fp->target_interval_us = 11111; break;
        case 120: fp->target_interval_us = 8333;  break;
        case 144: fp->target_interval_us = 6944;  break;
        case 240: fp->target_interval_us = 4166;  break;
        default:  fp->target_interval_us = 1000000ULL / hz; break;
    }

    return AGDTE_OK;
}

uint64_t AGDTE_Pacer_CalculateNextDeadline(uint32_t display_id, uint64_t current_time_us) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return current_time_us + 16666;
    }

    AGDTE_FramePacerState* fp = &s_pacers[display_id];
    if (!fp->active) {
        return current_time_us + 16666;
    }

    /* First frame after boot must present immediately without interval delay */
    if (fp->last_presentation_us == 0) {
        fp->next_presentation_deadline_us = current_time_us;
        return fp->next_presentation_deadline_us;
    }

    if (fp->next_presentation_deadline_us == 0 || fp->next_presentation_deadline_us <= current_time_us) {
        fp->next_presentation_deadline_us = current_time_us + fp->target_interval_us;
    } else {
        fp->next_presentation_deadline_us += fp->target_interval_us;
    }

    return fp->next_presentation_deadline_us;
}

AGDTE_PacerVerdict AGDTE_Pacer_EvaluateReadiness(uint32_t display_id, uint64_t current_time_us, uint64_t request_deadline_us) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return AGDTE_PACER_READY;
    }

    AGDTE_FramePacerState* fp = &s_pacers[display_id];
    if (!fp->active) {
        return AGDTE_PACER_READY;
    }

    /* Check against last presentation timestamp to prevent burst presentation (< 50% interval) */
    if (fp->last_presentation_us > 0 && current_time_us > fp->last_presentation_us) {
        uint64_t elapsed_since_last = current_time_us - fp->last_presentation_us;
        if (elapsed_since_last < (fp->target_interval_us / 2)) {
            return AGDTE_PACER_BURST;
        }
    }

    /* Check if current time has reached or passed deadline (within tolerance) */
    if (current_time_us + fp->jitter_tolerance_us >= request_deadline_us) {
        if (current_time_us > request_deadline_us + fp->target_interval_us) {
            return AGDTE_PACER_LATE;
        }
        return AGDTE_PACER_READY;
    }

    return AGDTE_PACER_WAIT;
}

AGDTE_Error AGDTE_Pacer_OnPresentationExecuted(uint32_t display_id, uint64_t present_time_us) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    AGDTE_FramePacerState* fp = &s_pacers[display_id];
    fp->last_presentation_us = present_time_us;
    if (fp->next_presentation_deadline_us <= present_time_us) {
        fp->next_presentation_deadline_us = present_time_us + fp->target_interval_us;
    }

    return AGDTE_OK;
}

AGDTE_FramePacerState* AGDTE_Pacer_GetState(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return 0;
    }
    return &s_pacers[display_id];
}
