/**
 * @file agdte_scheduler.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Deterministic Display Scheduler
 * @status Phase 2 Core Architecture & Display Scheduler Frozen
 *
 * @section PURPOSE
 * Decides exact presentation timing and cadence transitions. Evaluates when presentation
 * should wait, continue, merge dirty regions, skip superseded frames, or force immediate scanout.
 * The scheduler NEVER renders pixels; it only schedules presentation actions.
 */

#include "../include/agdte.h"

static uint64_t s_next_scheduled_time_us = 0;

AGDTE_SchedulerDecision AGDTE_Scheduler_Evaluate(const AGDTE_PresentRequest* req, uint64_t current_time_us) {
    if (!req) {
        return AGDTE_DECISION_NONE;
    }

    if (req->cancelled) {
        AGDTE_Diag_RecordDecision(AGDTE_DECISION_SKIP_SUPERSEDED);
        return AGDTE_DECISION_SKIP_SUPERSEDED;
    }

    /* Forced immediate presentation overrides cadence gating */
    if (req->force_immediate) {
        s_next_scheduled_time_us = current_time_us;
        AGDTE_Diag_RecordDecision(AGDTE_DECISION_FORCE_PRESENT);
        return AGDTE_DECISION_FORCE_PRESENT;
    }

    /* Critical hardware/software cursor presentation bypasses normal 60Hz frame pacing */
    if (req->priority == AGDTE_PRIORITY_CRITICAL_CURSOR) {
        s_next_scheduled_time_us = current_time_us;
        AGDTE_Diag_RecordDecision(AGDTE_DECISION_PRESENT_NOW);
        return AGDTE_DECISION_PRESENT_NOW;
    }

    /* Check cadence mode and target deadline */
    AGDTE_DisplayState* disp = AGDTE_Display_GetState(req->display_id);
    if (!disp || disp->cadence_mode == AGDTE_CADENCE_IMMEDIATE) {
        s_next_scheduled_time_us = current_time_us;
        AGDTE_Diag_RecordDecision(AGDTE_DECISION_PRESENT_NOW);
        return AGDTE_DECISION_PRESENT_NOW;
    }

    /* Check readiness with Phase 4 Deterministic Frame Pacer and VSync state */
    AGDTE_PacerVerdict pacer_verdict = AGDTE_Pacer_EvaluateReadiness(req->display_id, current_time_us, req->target_deadline_us);
    bool vsync_ready = AGDTE_VSync_IsSyncReady(req->display_id, current_time_us);

    if (!vsync_ready || pacer_verdict == AGDTE_PACER_WAIT || pacer_verdict == AGDTE_PACER_BURST) {
        s_next_scheduled_time_us = req->target_deadline_us;
        AGDTE_Diag_RecordDecision(AGDTE_DECISION_WAIT_PACING);
        return AGDTE_DECISION_WAIT_PACING;
    }

    s_next_scheduled_time_us = AGDTE_Timing_CalculateCadenceDeadline(req->display_id, current_time_us);
    AGDTE_Diag_RecordDecision(AGDTE_DECISION_PRESENT_NOW);
    return AGDTE_DECISION_PRESENT_NOW;
}

static void agdte_rect_union(BOGE_Rect* out, const BOGE_Rect* a, const BOGE_Rect* b) {
    int32_t min_x = (a->x < b->x) ? a->x : b->x;
    int32_t min_y = (a->y < b->y) ? a->y : b->y;
    
    int32_t max_x_a = a->x + (int32_t)a->width;
    int32_t max_x_b = b->x + (int32_t)b->width;
    int32_t max_x = (max_x_a > max_x_b) ? max_x_a : max_x_b;

    int32_t max_y_a = a->y + (int32_t)a->height;
    int32_t max_y_b = b->y + (int32_t)b->height;
    int32_t max_y = (max_y_a > max_y_b) ? max_y_a : max_y_b;

    out->x = min_x;
    out->y = min_y;
    out->width = (uint32_t)(max_x - min_x);
    out->height = (uint32_t)(max_y - min_y);
}

AGDTE_Error AGDTE_Scheduler_MergeDirtyRegions(AGDTE_PresentRequest* target, const AGDTE_PresentRequest* source) {
    if (!target || !source) {
        return AGDTE_ERR_NULL_POINTER;
    }

    /* If source requires full-screen repaint (dirty_count == 0), target inherits full repaint */
    if (source->dirty_count == 0) {
        target->dirty_count = 0;
        return AGDTE_OK;
    }
    if (target->dirty_count == 0) {
        return AGDTE_OK; /* Target already full-screen dirty */
    }

    /* Append or merge bounding rects without exceeding AGDTE_MAX_DIRTY_RECTS */
    for (uint32_t i = 0; i < source->dirty_count; i++) {
        if (target->dirty_count < AGDTE_MAX_DIRTY_RECTS) {
            target->dirty_rects[target->dirty_count++] = source->dirty_rects[i];
        } else {
            /* Pool saturated: compute bounding box union into slot 0 and shrink count */
            BOGE_Rect union_rect = target->dirty_rects[0];
            for (uint32_t j = 1; j < target->dirty_count; j++) {
                agdte_rect_union(&union_rect, &union_rect, &target->dirty_rects[j]);
            }
            agdte_rect_union(&union_rect, &union_rect, &source->dirty_rects[i]);
            target->dirty_rects[0] = union_rect;
            target->dirty_count = 1;
        }
    }

    /* Update frame ID and deadline to reflect the latest incoming request */
    target->frame_id = source->frame_id;
    if (source->submit_time_us > target->submit_time_us) {
        target->submit_time_us = source->submit_time_us;
    }
    return AGDTE_OK;
}

uint64_t AGDTE_Scheduler_GetNextScheduledTimeUs(void) {
    return s_next_scheduled_time_us;
}
