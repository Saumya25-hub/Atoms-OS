/**
 * @file agdte_presenter.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Presentation Execution Engine
 * @status Phase 2 Core Architecture & Display Scheduler Frozen
 *
 * @section PURPOSE
 * Executes physical presentation of scheduled surfaces to target displays.
 * Interfaces directly with the active Hardware Backend Abstraction (`AGDTE_BackendOps`)
 * or bridges transparently to the legacy/BSPE Dual-Page Presentation Engine.
 */

#include "../include/agdte.h"

extern uint64_t timer_get_ticks(void);

AGDTE_Error AGDTE_Presenter_Execute(const AGDTE_PresentRequest* req, uint64_t current_time_us) {
    if (!req) {
        return AGDTE_ERR_NULL_POINTER;
    }

    uint64_t exec_start_us = timer_get_ticks() * 1000ULL;
    AGDTE_Timeline_RecordPresent(req->frame_id, exec_start_us);
    AGDTE_SwapController_CommitSwap(req->display_id, req->buffer_id, exec_start_us);

    AGDTE_DisplayState* disp = AGDTE_Display_GetState(req->display_id);
    if (!disp || !disp->active) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    AGDTE_BufferDescriptor* buf = AGDTE_Buffer_GetDescriptor(req->buffer_id);
    if (!buf || !buf->virtual_address) {
        return AGDTE_ERR_INVALID_BUFFER;
    }

    const AGDTE_BackendOps* ops = AGDTE_Backend_GetOps(disp->backend_type);
    if (!ops || !ops->present_buffer) {
        return AGDTE_ERR_BACKEND_FAILED;
    }

    /* Transfer buffer ownership temporarily to display scanout while active */
    AGDTE_Buffer_TransferOwnership(req->buffer_id, AGDTE_BUFFER_OWNER_DISPLAY_ACTIVE);
    disp->active_frontbuffer_id = req->buffer_id;

    /* Execute physical presentation via backend abstraction */
    AGDTE_Error err = ops->present_buffer(req->display_id, buf, req->dirty_rects, req->dirty_count);
    if (err != AGDTE_OK) {
        return err;
    }

    if (ops->flip_page) {
        ops->flip_page(req->display_id, req->buffer_id);
    }

    /* Record exact presentation timing and latency */
    AGDTE_Timing_RecordPresentation(req->request_id, current_time_us);

    uint64_t exec_end_us = timer_get_ticks() * 1000ULL;
    uint64_t duration_us = (exec_end_us >= exec_start_us) ? (exec_end_us - exec_start_us) : 0;
    AGDTE_Diag_RecordLatency(duration_us);
    AGDTE_Diag_RecordLayerPresentTime(req->target_layer, duration_us);

    AGDTE_Timeline_RecordDisplay(req->frame_id, exec_end_us);
    AGDTE_Timeline_RecordCompletion(req->frame_id, exec_end_us);
    AGDTE_TimelineEntry* entry = AGDTE_Timeline_GetEntry(req->frame_id);
    if (entry) {
        AGDTE_Metrics_OnFrameComplete(entry, AGDTE_RefreshController_GetIntervalUs(req->display_id));
    }

    return AGDTE_OK;
}

AGDTE_Error AGDTE_Presenter_PresentBridgeBSPE(const BOGE_StagingFrame* boge_frame, uint32_t display_id) {
    if (!boge_frame || !boge_frame->buffer_virtual_address) {
        return AGDTE_ERR_NULL_POINTER;
    }

    /* Ensure AGDTE is initialized; if not, pass straight to BSPE for 100% backward compatibility */
    if (!AGDTE_IsInitialized()) {
        BSPE_Error bspe_err = BSPE_PresentFrame(boge_frame);
        return (bspe_err == BSPE_OK) ? AGDTE_OK : AGDTE_ERR_BACKEND_FAILED;
    }

    uint64_t current_time_us = timer_get_ticks() * 1000ULL;
    AGDTE_Timeline_RecordSubmit(boge_frame->frame_id, display_id, AGDTE_LAYER_WINDOWS, current_time_us);

    /* Register temporary buffer handle inside AGDTE buffer pool */
    uint32_t buffer_id = 0xFFFFFFFF;
    AGDTE_Error err = AGDTE_Buffer_Register(
        boge_frame->buffer_virtual_address,
        boge_frame->width,
        boge_frame->height,
        boge_frame->pitch,
        AGDTE_BUFFER_ROLE_STAGING,
        &buffer_id
    );

    if (err != AGDTE_OK) {
        /* Fallback if static buffer pool saturated */
        BSPE_Error bspe_err = BSPE_PresentFrame(boge_frame);
        return (bspe_err == BSPE_OK) ? AGDTE_OK : AGDTE_ERR_BACKEND_FAILED;
    }

    /* Assign buffer to our registered Windows surface plane if present */
    uint32_t win_surf_id = 0;
    if (AGDTE_Surface_GetLayerSurfaceID(AGDTE_LAYER_WINDOWS, &win_surf_id) == AGDTE_OK) {
        AGDTE_Surface_AssignBuffer(win_surf_id, buffer_id);
    }

    /* Construct presentation request with deterministic pacer deadline */
    AGDTE_PresentRequest req;
    req.request_id = 0;
    req.frame_id = boge_frame->frame_id;
    req.display_id = display_id;
    req.buffer_id = buffer_id;
    req.priority = AGDTE_PRIORITY_NORMAL;
    req.target_layer = AGDTE_LAYER_WINDOWS;
    req.submit_time_us = current_time_us;
    req.target_deadline_us = AGDTE_Pacer_CalculateNextDeadline(display_id, current_time_us);
    req.dirty_count = (boge_frame->dirty_count <= AGDTE_MAX_DIRTY_RECTS) ? boge_frame->dirty_count : AGDTE_MAX_DIRTY_RECTS;
    for (uint32_t i = 0; i < req.dirty_count; i++) {
        req.dirty_rects[i] = boge_frame->dirty_rects[i];
    }
    req.allow_skip = true;
    req.force_immediate = false;
    req.cancelled = false;

    uint32_t req_id = 0;
    AGDTE_Queue_Submit(&req, &req_id);
    AGDTE_Timeline_RecordQueue(boge_frame->frame_id, current_time_us);
    AGDTE_Timing_RecordSubmit(req_id, req.submit_time_us, req.target_deadline_us);
    AGDTE_SwapController_SubmitBuffer(display_id, buffer_id, req_id, req.target_deadline_us);

    /* Evaluate scheduler and execute if ready */
    AGDTE_PresentRequest popped;
    if (AGDTE_Queue_PopNext(&popped) == AGDTE_OK) {
        AGDTE_SchedulerDecision decision = AGDTE_Scheduler_Evaluate(&popped, current_time_us);
        if (decision == AGDTE_DECISION_PRESENT_NOW || decision == AGDTE_DECISION_FORCE_PRESENT) {
            AGDTE_Timeline_RecordSchedule(boge_frame->frame_id, current_time_us);
            AGDTE_Presenter_Execute(&popped, current_time_us);
            AGDTE_Buffer_Unregister(buffer_id);
        } else if (decision == AGDTE_DECISION_SKIP_SUPERSEDED) {
            AGDTE_Timeline_RecordSchedule(boge_frame->frame_id, current_time_us);
            AGDTE_Metrics_RecordSkipped();
            AGDTE_Buffer_Unregister(buffer_id);
        } else {
            /* Retain request in queue for subsequent pulse evaluation without unregistering buffer */
            AGDTE_Timeline_RecordSchedule(boge_frame->frame_id, popped.target_deadline_us);
            AGDTE_Queue_Submit(&popped, &req_id);
        }
    }

    return AGDTE_OK;
}
