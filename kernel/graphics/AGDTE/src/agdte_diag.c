/**
 * @file agdte_diag.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Diagnostics Engine
 * @status Phase 2 Core Architecture & Display Scheduler Frozen
 *
 * @section PURPOSE
 * High-precision diagnostic tracking for the master display controller without heap
 * allocations or floating point arithmetic. Records queue depths, scheduler verdicts,
 * presentation latency, buffer copies, and worst-case timing outliers.
 */

#include "../include/agdte.h"

static AGDTE_Diagnostics s_diagnostics;
static uint64_t s_depth_sample_count = 0;
static uint64_t s_depth_accum_x100 = 0;
static uint64_t s_latency_accum_us = 0;

void AGDTE_Diag_Init(void) {
    s_diagnostics.frames_submitted = 0;
    s_diagnostics.frames_scheduled = 0;
    s_diagnostics.frames_presented = 0;
    s_diagnostics.frames_skipped = 0;
    s_diagnostics.frames_delayed = 0;
    s_diagnostics.frames_batched = 0;
    s_diagnostics.queue_usage_current = 0;
    s_diagnostics.queue_depth_max = 0;
    s_diagnostics.queue_depth_avg_x100 = 0;
    for (uint32_t i = 0; i < 6; i++) {
        s_diagnostics.scheduler_decisions[i] = 0;
    }
    s_diagnostics.present_time_last_us = 0;
    s_diagnostics.present_time_worst_us = 0;
    s_diagnostics.present_time_avg_us = 0;
    s_diagnostics.pulse_time_last_us = 0;
    s_diagnostics.pulse_time_worst_us = 0;
    s_diagnostics.cursor_present_time_us = 0;
    s_diagnostics.window_present_time_us = 0;
    s_diagnostics.surface_count = 0;
    s_diagnostics.buffer_copies = 0;
    s_diagnostics.dirty_rect_merge_count = 0;
    s_diagnostics.active_backend = AGDTE_BACKEND_VBE;
    s_diagnostics.presentation_latency_us = 0;

    s_depth_sample_count = 0;
    s_depth_accum_x100 = 0;
    s_latency_accum_us = 0;
}

void AGDTE_Diag_RecordDecision(AGDTE_SchedulerDecision decision) {
    if ((uint32_t)decision < 6) {
        s_diagnostics.scheduler_decisions[(uint32_t)decision]++;
    }
    if (decision == AGDTE_DECISION_SKIP_SUPERSEDED) {
        s_diagnostics.frames_skipped++;
    } else if (decision == AGDTE_DECISION_WAIT_PACING) {
        s_diagnostics.frames_delayed++;
    } else if (decision == AGDTE_DECISION_MERGE_BATCH) {
        s_diagnostics.frames_batched++;
        s_diagnostics.dirty_rect_merge_count++;
    } else if (decision == AGDTE_DECISION_PRESENT_NOW || decision == AGDTE_DECISION_FORCE_PRESENT) {
        s_diagnostics.frames_submitted++;
        s_diagnostics.frames_scheduled++;
    }
}

void AGDTE_Diag_RecordQueueDepth(uint32_t current_depth) {
    s_diagnostics.queue_usage_current = current_depth;
    if (current_depth > s_diagnostics.queue_depth_max) {
        s_diagnostics.queue_depth_max = current_depth;
    }

    s_depth_sample_count++;
    s_depth_accum_x100 += (uint64_t)current_depth * 100;
    if (s_depth_sample_count > 0) {
        s_diagnostics.queue_depth_avg_x100 = (uint32_t)(s_depth_accum_x100 / s_depth_sample_count);
    }
}

void AGDTE_Diag_RecordLatency(uint64_t duration_us) {
    s_diagnostics.present_time_last_us = duration_us;
    s_diagnostics.presentation_latency_us = duration_us;
    if (duration_us > s_diagnostics.present_time_worst_us) {
        s_diagnostics.present_time_worst_us = duration_us;
    }
    s_diagnostics.frames_presented++;
    s_latency_accum_us += duration_us;
    if (s_diagnostics.frames_presented > 0) {
        s_diagnostics.present_time_avg_us = s_latency_accum_us / s_diagnostics.frames_presented;
    }
}

void AGDTE_Diag_RecordPulseTime(uint64_t duration_us) {
    s_diagnostics.pulse_time_last_us = duration_us;
    if (duration_us > s_diagnostics.pulse_time_worst_us) {
        s_diagnostics.pulse_time_worst_us = duration_us;
    }
}

void AGDTE_Diag_RecordLayerPresentTime(AGDTE_SurfaceLayer layer, uint64_t duration_us) {
    if (layer == AGDTE_LAYER_CURSOR) {
        s_diagnostics.cursor_present_time_us = duration_us;
    } else if (layer == AGDTE_LAYER_WINDOWS || layer == AGDTE_LAYER_DESKTOP) {
        s_diagnostics.window_present_time_us = duration_us;
    }
}

void AGDTE_Diag_UpdateSurfaceCount(uint32_t count) {
    s_diagnostics.surface_count = count;
}

AGDTE_Diagnostics* AGDTE_Diag_GetSnapshot(void) {
    return &s_diagnostics;
}

void AGDTE_Diag_DumpConsole(void) {
    /* Safe hook: print diagnostics if kernel console active */
}
