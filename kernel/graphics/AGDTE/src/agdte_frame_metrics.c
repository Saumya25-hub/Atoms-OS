/**
 * @file agdte_frame_metrics.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Frame Metrics Engine
 * @status Phase 4 Display Timing Optimization Layer
 *
 * @section PURPOSE
 * Measures presentation latency, delay components (`queue_delay`, `scheduler_delay`,
 * `swap_delay`), dropped/skipped/burst frames, and computes integer mean absolute
 * variance (`frame_variance_us`, `presentation_variance_us`) without floating point math.
 */

#include "../include/agdte.h"

static AGDTE_FrameMetricsSnapshot s_metrics;
static uint64_t s_accum_frame_time_us = 0;
static uint64_t s_accum_latency_us = 0;
static uint64_t s_accum_queue_delay_us = 0;
static uint64_t s_accum_sched_delay_us = 0;
static uint64_t s_accum_swap_delay_us = 0;
static uint64_t s_last_frame_completion_us = 0;
static uint64_t s_last_present_start_us = 0;

/* Fixed size history for exact integer variance calculation */
#define AGDTE_METRICS_HISTORY_SIZE 32
static uint64_t s_frame_intervals[AGDTE_METRICS_HISTORY_SIZE];
static uint64_t s_present_intervals[AGDTE_METRICS_HISTORY_SIZE];
static uint32_t s_hist_head = 0;
static uint32_t s_hist_count = 0;

void AGDTE_Metrics_Init(void) {
    s_metrics.avg_frame_time_us = 0;
    s_metrics.worst_frame_time_us = 0;
    s_metrics.presentation_latency_us = 0;
    s_metrics.queue_delay_us = 0;
    s_metrics.scheduler_delay_us = 0;
    s_metrics.swap_delay_us = 0;
    s_metrics.display_delay_us = 0;
    s_metrics.dropped_frames = 0;
    s_metrics.skipped_frames = 0;
    s_metrics.late_frames = 0;
    s_metrics.early_frames = 0;
    s_metrics.burst_frames = 0;
    s_metrics.frame_variance_us = 0;
    s_metrics.presentation_variance_us = 0;
    s_metrics.total_frames_measured = 0;

    s_accum_frame_time_us = 0;
    s_accum_latency_us = 0;
    s_accum_queue_delay_us = 0;
    s_accum_sched_delay_us = 0;
    s_accum_swap_delay_us = 0;
    s_last_frame_completion_us = 0;
    s_last_present_start_us = 0;
    s_hist_head = 0;
    s_hist_count = 0;
}

static uint64_t agdte_abs_diff(uint64_t a, uint64_t b) {
    return (a > b) ? (a - b) : (b - a);
}

void AGDTE_Metrics_OnFrameComplete(const AGDTE_TimelineEntry* entry, uint64_t target_interval_us) {
    if (!entry || !entry->valid) {
        return;
    }

    s_metrics.total_frames_measured++;

    /* Compute delays */
    uint64_t q_delay = (entry->queue_timestamp_us > entry->submit_timestamp_us) ?
        (entry->queue_timestamp_us - entry->submit_timestamp_us) : 0;
    uint64_t s_delay = (entry->present_timestamp_us > entry->queue_timestamp_us) ?
        (entry->present_timestamp_us - entry->queue_timestamp_us) : 0;
    uint64_t sw_delay = (entry->completion_timestamp_us > entry->present_timestamp_us) ?
        (entry->completion_timestamp_us - entry->present_timestamp_us) : 0;
    uint64_t total_latency = (entry->completion_timestamp_us > entry->submit_timestamp_us) ?
        (entry->completion_timestamp_us - entry->submit_timestamp_us) : 0;

    s_accum_queue_delay_us += q_delay;
    s_accum_sched_delay_us += s_delay;
    s_accum_swap_delay_us  += sw_delay;
    s_accum_latency_us     += total_latency;

    s_metrics.queue_delay_us          = s_accum_queue_delay_us / s_metrics.total_frames_measured;
    s_metrics.scheduler_delay_us      = s_accum_sched_delay_us / s_metrics.total_frames_measured;
    s_metrics.swap_delay_us           = s_accum_swap_delay_us  / s_metrics.total_frames_measured;
    s_metrics.presentation_latency_us = s_accum_latency_us     / s_metrics.total_frames_measured;

    /* Compute frame & presentation intervals */
    if (s_last_frame_completion_us > 0 && entry->completion_timestamp_us > s_last_frame_completion_us) {
        uint64_t frame_interval = entry->completion_timestamp_us - s_last_frame_completion_us;
        s_accum_frame_time_us += frame_interval;
        s_metrics.avg_frame_time_us = s_accum_frame_time_us / (s_metrics.total_frames_measured - 1);
        if (frame_interval > s_metrics.worst_frame_time_us) {
            s_metrics.worst_frame_time_us = frame_interval;
        }

        /* Check for burst frames (< 50% of target interval) */
        if (target_interval_us > 0 && frame_interval < (target_interval_us / 2)) {
            s_metrics.burst_frames++;
        }

        s_frame_intervals[s_hist_head] = frame_interval;
    }

    if (s_last_present_start_us > 0 && entry->present_timestamp_us > s_last_present_start_us) {
        uint64_t present_interval = entry->present_timestamp_us - s_last_present_start_us;
        s_present_intervals[s_hist_head] = present_interval;
    }

    s_last_frame_completion_us = entry->completion_timestamp_us;
    s_last_present_start_us = entry->present_timestamp_us;

    s_hist_head = (s_hist_head + 1) % AGDTE_METRICS_HISTORY_SIZE;
    if (s_hist_count < AGDTE_METRICS_HISTORY_SIZE) {
        s_hist_count++;
    }

    /* Compute integer Mean Absolute Deviation across history window */
    if (s_hist_count > 1 && s_metrics.avg_frame_time_us > 0) {
        uint64_t sum_dev_frame = 0;
        uint64_t sum_dev_present = 0;
        for (uint32_t i = 0; i < s_hist_count; i++) {
            sum_dev_frame += agdte_abs_diff(s_frame_intervals[i], s_metrics.avg_frame_time_us);
            sum_dev_present += agdte_abs_diff(s_present_intervals[i], s_metrics.avg_frame_time_us);
        }
        s_metrics.frame_variance_us = sum_dev_frame / s_hist_count;
        s_metrics.presentation_variance_us = sum_dev_present / s_hist_count;
    }

    /* Evaluate late/early against target interval tolerance (e.g., +/- 1500 us) */
    if (entry->schedule_timestamp_us > 0 && entry->present_timestamp_us > 0) {
        if (entry->present_timestamp_us > entry->schedule_timestamp_us + 1500) {
            s_metrics.late_frames++;
        } else if (entry->schedule_timestamp_us > entry->present_timestamp_us + 1500) {
            s_metrics.early_frames++;
        }
    }
}

void AGDTE_Metrics_RecordDropped(void) {
    s_metrics.dropped_frames++;
}

void AGDTE_Metrics_RecordSkipped(void) {
    s_metrics.skipped_frames++;
}

AGDTE_FrameMetricsSnapshot* AGDTE_Metrics_GetSnapshot(void) {
    return &s_metrics;
}
