/**
 * @file agdte_swap_controller.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Presentation Swap Controller
 * @status Phase 4 Display Timing Optimization Layer
 *
 * @section PURPOSE
 * Manages page flip scheduling, swap timing decisions (`EvaluateSwap`), and triple
 * buffer ownership states (`front_buffer`, `back_buffer`, `pending_buffer`). Prevents
 * duplicate/redundant page flips and prepares future asynchronous presentation.
 */

#include "../include/agdte.h"

static AGDTE_SwapControllerState s_swaps[AGDTE_MAX_DISPLAYS];

AGDTE_Error AGDTE_SwapController_Init(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    AGDTE_SwapControllerState* sc = &s_swaps[display_id];
    sc->display_id = display_id;
    sc->front_buffer_id = 0xFFFFFFFF;
    sc->back_buffer_id  = 0xFFFFFFFF;
    sc->pending_buffer_id = 0xFFFFFFFF;
    sc->pending_request_id = 0xFFFFFFFF;
    sc->pending_deadline_us = 0;
    sc->swap_pending = false;
    sc->last_swap_time_us = 0;
    sc->total_swaps_executed = 0;
    sc->duplicate_swaps_prevented = 0;

    return AGDTE_OK;
}

AGDTE_Error AGDTE_SwapController_SubmitBuffer(uint32_t display_id, uint32_t buffer_id, uint32_t request_id, uint64_t deadline_us) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    AGDTE_SwapControllerState* sc = &s_swaps[display_id];
    
    /* If same buffer already active or staging, avoid duplicate swap registration */
    if (buffer_id == sc->front_buffer_id && !sc->swap_pending) {
        sc->duplicate_swaps_prevented++;
        return AGDTE_OK;
    }

    /* Assign to pending slot in triple buffer architecture */
    sc->pending_buffer_id = buffer_id;
    sc->pending_request_id = request_id;
    sc->pending_deadline_us = deadline_us;
    sc->swap_pending = true;

    return AGDTE_OK;
}

AGDTE_SwapDecision AGDTE_SwapController_EvaluateSwap(uint32_t display_id, uint64_t current_time_us, uint32_t* out_buffer_id, uint32_t* out_request_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS || !out_buffer_id || !out_request_id) {
        return AGDTE_SWAP_DECISION_DISCARD;
    }

    AGDTE_SwapControllerState* sc = &s_swaps[display_id];
    if (!sc->swap_pending || sc->pending_buffer_id == 0xFFFFFFFF) {
        return AGDTE_SWAP_DECISION_HOLD;
    }

    /* Check if target buffer is valid and distinct from current active frontbuffer */
    if (sc->pending_buffer_id == sc->front_buffer_id && sc->last_swap_time_us > 0) {
        sc->duplicate_swaps_prevented++;
        sc->swap_pending = false;
        return AGDTE_SWAP_DECISION_DISCARD;
    }

    /* Check readiness with Frame Pacer and VSync state */
    AGDTE_PacerVerdict verdict = AGDTE_Pacer_EvaluateReadiness(display_id, current_time_us, sc->pending_deadline_us);
    if (verdict == AGDTE_PACER_WAIT) {
        return AGDTE_SWAP_DECISION_HOLD;
    }

    *out_buffer_id  = sc->pending_buffer_id;
    *out_request_id = sc->pending_request_id;
    return AGDTE_SWAP_DECISION_COMMIT;
}

AGDTE_Error AGDTE_SwapController_CommitSwap(uint32_t display_id, uint32_t buffer_id, uint64_t present_time_us) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    AGDTE_SwapControllerState* sc = &s_swaps[display_id];

    /* Triple buffer rotation: front -> free, back -> front, pending -> back */
    sc->back_buffer_id  = sc->front_buffer_id;
    sc->front_buffer_id = buffer_id;
    if (sc->pending_buffer_id == buffer_id) {
        sc->pending_buffer_id = 0xFFFFFFFF;
        sc->swap_pending = false;
    }

    sc->last_swap_time_us = present_time_us;
    sc->total_swaps_executed++;

    AGDTE_Pacer_OnPresentationExecuted(display_id, present_time_us);

    return AGDTE_OK;
}

AGDTE_SwapControllerState* AGDTE_SwapController_GetState(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return 0;
    }
    return &s_swaps[display_id];
}
