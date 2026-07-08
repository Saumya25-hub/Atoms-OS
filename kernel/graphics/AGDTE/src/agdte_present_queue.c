/**
 * @file agdte_present_queue.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Presentation Queue
 * @status Phase 2 Core Architecture & Display Scheduler Frozen
 *
 * @section PURPOSE
 * Implements a deterministic, zero-allocation priority/FIFO presentation request queue.
 * Supports priority levels, future deadlines, frame IDs, cancellation, and batching.
 */

#include "../include/agdte.h"

/* --- Static Storage for Presentation Queue --- */
static AGDTE_PresentRequest s_queue[AGDTE_MAX_QUEUE_DEPTH];
static uint32_t s_queue_count = 0;
static uint32_t s_next_request_id = 1;

void agdte_queue_reset_all(void) {
    for (uint32_t i = 0; i < AGDTE_MAX_QUEUE_DEPTH; i++) {
        s_queue[i].request_id = 0;
        s_queue[i].frame_id = 0;
        s_queue[i].display_id = 0;
        s_queue[i].buffer_id = 0xFFFFFFFF;
        s_queue[i].priority = AGDTE_PRIORITY_NORMAL;
        s_queue[i].target_layer = AGDTE_LAYER_DESKTOP;
        s_queue[i].submit_time_us = 0;
        s_queue[i].target_deadline_us = 0;
        s_queue[i].dirty_count = 0;
        s_queue[i].allow_skip = true;
        s_queue[i].force_immediate = false;
        s_queue[i].cancelled = false;
    }
    s_queue_count = 0;
    s_next_request_id = 1;
}

AGDTE_Error AGDTE_Queue_Submit(const AGDTE_PresentRequest* req, uint32_t* out_request_id) {
    if (!req) {
        return AGDTE_ERR_NULL_POINTER;
    }
    if (s_queue_count >= AGDTE_MAX_QUEUE_DEPTH) {
        return AGDTE_ERR_QUEUE_FULL;
    }

    uint32_t req_id = s_next_request_id++;
    if (s_next_request_id == 0) {
        s_next_request_id = 1; /* Handle overflow */
    }

    /* Check if batching possible with an existing queued request of exact same display & layer */
    for (uint32_t i = 0; i < s_queue_count; i++) {
        AGDTE_PresentRequest* q = &s_queue[i];
        if (!q->cancelled && q->display_id == req->display_id && q->target_layer == req->target_layer && q->buffer_id == req->buffer_id) {
            /* Try to merge dirty rects into existing queued item if batching enabled */
            if (req->allow_skip && q->allow_skip) {
                AGDTE_Scheduler_MergeDirtyRegions(q, req);
                if (out_request_id) {
                    *out_request_id = q->request_id;
                }
                AGDTE_Diag_RecordDecision(AGDTE_DECISION_MERGE_BATCH);
                return AGDTE_OK;
            }
        }
    }

    /* Find insertion slot to maintain Priority (high -> low) then FIFO order */
    uint32_t insert_idx = s_queue_count;
    for (uint32_t i = 0; i < s_queue_count; i++) {
        if (req->priority > s_queue[i].priority) {
            insert_idx = i;
            break;
        }
    }

    /* Shift items down to make space at insert_idx */
    for (uint32_t j = s_queue_count; j > insert_idx; j--) {
        s_queue[j] = s_queue[j - 1];
    }

    s_queue[insert_idx] = *req;
    s_queue[insert_idx].request_id = req_id;
    s_queue[insert_idx].cancelled = false;

    /* Clamp dirty rect count to static maximum */
    if (s_queue[insert_idx].dirty_count > AGDTE_MAX_DIRTY_RECTS) {
        s_queue[insert_idx].dirty_count = AGDTE_MAX_DIRTY_RECTS;
    }

    s_queue_count++;
    if (out_request_id) {
        *out_request_id = req_id;
    }

    AGDTE_Diag_RecordQueueDepth(s_queue_count);
    return AGDTE_OK;
}

AGDTE_Error AGDTE_Queue_PopNext(AGDTE_PresentRequest* out_req) {
    if (!out_req) {
        return AGDTE_ERR_NULL_POINTER;
    }
    if (s_queue_count == 0) {
        return AGDTE_ERR_QUEUE_EMPTY;
    }

    /* Skip cancelled requests */
    while (s_queue_count > 0 && s_queue[0].cancelled) {
        for (uint32_t j = 0; j < s_queue_count - 1; j++) {
            s_queue[j] = s_queue[j + 1];
        }
        s_queue_count--;
    }

    if (s_queue_count == 0) {
        return AGDTE_ERR_QUEUE_EMPTY;
    }

    *out_req = s_queue[0];
    for (uint32_t j = 0; j < s_queue_count - 1; j++) {
        s_queue[j] = s_queue[j + 1];
    }
    s_queue_count--;

    AGDTE_Diag_RecordQueueDepth(s_queue_count);
    return AGDTE_OK;
}

AGDTE_Error AGDTE_Queue_PeekNext(AGDTE_PresentRequest* out_req) {
    if (!out_req) {
        return AGDTE_ERR_NULL_POINTER;
    }
    for (uint32_t i = 0; i < s_queue_count; i++) {
        if (!s_queue[i].cancelled) {
            *out_req = s_queue[i];
            return AGDTE_OK;
        }
    }
    return AGDTE_ERR_QUEUE_EMPTY;
}

AGDTE_Error AGDTE_Queue_CancelRequest(uint32_t request_id) {
    for (uint32_t i = 0; i < s_queue_count; i++) {
        if (s_queue[i].request_id == request_id && !s_queue[i].cancelled) {
            s_queue[i].cancelled = true;
            return AGDTE_OK;
        }
    }
    return AGDTE_ERR_INVALID_STATE;
}

AGDTE_Error AGDTE_Queue_Flush(void) {
    s_queue_count = 0;
    AGDTE_Diag_RecordQueueDepth(0);
    return AGDTE_OK;
}

uint32_t AGDTE_Queue_GetDepth(void) {
    uint32_t active_count = 0;
    for (uint32_t i = 0; i < s_queue_count; i++) {
        if (!s_queue[i].cancelled) {
            active_count++;
        }
    }
    return active_count;
}
