/*
 * ============================================================================
 * ATOMS OS — Monotonic Media Clock & Presentation Deadline Scheduler
 * userspace/libbos_media/src/bos_media_clock.c
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements:
 * - Realtime monotonic stream clock based on SYS_UPTIME & rdtsc
 * - Presentation deadline evaluation & pacing state machine
 * - Bounded Decoded Frame Queue (DFQ)
 * - Mathematically justified reference-safe frame drop policy
 * ============================================================================
 */

#include "../include/bos_media_clock.h"
#include "userspace/runtime/c/include/atoms_syscall.h"
#include <string.h>

extern void display_print(const char* s);

static inline uint64_t get_system_time_us(void) {
    uint64_t uptime_ms = __atoms_syscall0(SYS_UPTIME);
    return uptime_ms * 1000ULL;
}

/*
 * Monotonic Media Clock Implementation
 */
void bos_media_clock_init(BOSMediaClock* clock) {
    if (!clock) return;
    memset(clock, 0, sizeof(BOSMediaClock));
}

void bos_media_clock_start(BOSMediaClock* clock, int64_t start_pts_us) {
    if (!clock) return;
    clock->start_time_us = get_system_time_us();
    clock->base_offset_us = start_pts_us;
    clock->is_paused = false;
    clock->is_running = true;
}

void bos_media_clock_pause(BOSMediaClock* clock) {
    if (!clock || !clock->is_running || clock->is_paused) return;
    clock->paused_at_us = get_system_time_us();
    clock->is_paused = true;
}

void bos_media_clock_resume(BOSMediaClock* clock) {
    if (!clock || !clock->is_running || !clock->is_paused) return;
    uint64_t now = get_system_time_us();
    if (now > clock->paused_at_us) {
        clock->start_time_us += (now - clock->paused_at_us);
    }
    clock->is_paused = false;
}

void bos_media_clock_seek(BOSMediaClock* clock, int64_t new_pts_us) {
    if (!clock) return;
    clock->start_time_us = get_system_time_us();
    clock->base_offset_us = new_pts_us;
    clock->paused_at_us = clock->start_time_us;
}

int64_t bos_media_clock_get_time_us(BOSMediaClock* clock) {
    if (!clock || !clock->is_running) return 0;
    if (clock->is_paused) {
        return (int64_t)(clock->paused_at_us - clock->start_time_us) + clock->base_offset_us;
    }
    uint64_t now = get_system_time_us();
    int64_t elapsed = (int64_t)(now - clock->start_time_us);
    if (elapsed < 0) elapsed = 0;
    return elapsed + clock->base_offset_us;
}

void bos_media_clock_observe_audio(BOSMediaClock* clock, int64_t audio_pts_us) {
    if (!clock) return;
    clock->observed_audio_clock_us = audio_pts_us;
}

/*
 * Bounded Decoded Frame Queue Implementation
 */
void bos_frame_queue_init(BOSFrameQueue* q) {
    if (!q) return;
    memset(q, 0, sizeof(BOSFrameQueue));
    q->low_water = 0;
    q->high_water = 0;
}

bool bos_frame_queue_push(BOSFrameQueue* q, const BOSDecodedFrame* frame) {
    if (!q || !frame) return false;
    if (q->count >= BOS_MAX_DECODED_FRAMES) return false;

    q->frames[q->write_idx] = *frame;
    q->write_idx = (q->write_idx + 1) % BOS_MAX_DECODED_FRAMES;
    q->count++;

    if (q->count > q->high_water) {
        q->high_water = q->count;
    }
    return true;
}

bool bos_frame_queue_pop(BOSFrameQueue* q, BOSDecodedFrame* out_frame) {
    if (!q || q->count == 0) return false;

    if (out_frame) {
        *out_frame = q->frames[q->read_idx];
    }
    q->read_idx = (q->read_idx + 1) % BOS_MAX_DECODED_FRAMES;
    q->count--;

    if (q->count < q->low_water) {
        q->low_water = q->count;
    }
    return true;
}

BOSDecodedFrame* bos_frame_queue_peek(BOSFrameQueue* q) {
    if (!q || q->count == 0) return NULL;
    return &q->frames[q->read_idx];
}

bool bos_frame_queue_is_full(const BOSFrameQueue* q) {
    return q ? (q->count >= BOS_MAX_DECODED_FRAMES) : true;
}

bool bos_frame_queue_is_empty(const BOSFrameQueue* q) {
    return q ? (q->count == 0) : true;
}

void bos_frame_queue_clear(BOSFrameQueue* q) {
    if (!q) return;
    q->read_idx = 0;
    q->write_idx = 0;
    q->count = 0;
}

/*
 * Presentation Scheduler Implementation
 */
void bos_media_scheduler_init(BOSMediaScheduler* s) {
    if (!s) return;
    memset(s, 0, sizeof(BOSMediaScheduler));
    bos_media_clock_init(&s->clock);
    bos_frame_queue_init(&s->frame_queue);
}

void bos_media_scheduler_start(BOSMediaScheduler* s, int64_t start_pts_us) {
    if (!s) return;
    bos_media_clock_start(&s->clock, start_pts_us);
    bos_frame_queue_clear(&s->frame_queue);
}

BOSMediaDeadlineState bos_media_scheduler_evaluate_deadline(BOSMediaScheduler* s, int64_t frame_pts_us, int64_t* out_delta_us) {
    if (!s) return BOS_DEADLINE_READY;

    int64_t media_time = bos_media_clock_get_time_us(&s->clock);
    int64_t delta_us = frame_pts_us - media_time;
    if (out_delta_us) *out_delta_us = delta_us;

    s->current_frame_deadline_delta_us = delta_us;

    if (delta_us > 10000) {
        // More than 10ms ahead -> hold frame
        s->current_deadline_state = BOS_DEADLINE_EARLY;
    } else if (delta_us >= -10000) {
        // Within [-10ms, +10ms] deadline window -> present now
        s->current_deadline_state = BOS_DEADLINE_READY;
    } else if (delta_us >= -50000) {
        // 10ms to 50ms behind -> late presentation
        s->current_deadline_state = BOS_DEADLINE_LATE;
    } else {
        // More than 50ms behind -> severely late
        s->current_deadline_state = BOS_DEADLINE_SEVERELY_LATE;
    }

    return s->current_deadline_state;
}

bool bos_media_scheduler_should_drop_frame(BOSMediaScheduler* s, const BOSDecodedFrame* frame, int64_t delta_us) {
    if (!s || !frame) return false;

    // Rule 1: Never drop reference frames (would corrupt future P/B frames)
    if (frame->is_reference || frame->is_keyframe) {
        return false;
    }

    // Rule 2: Only drop when severely late (> 50ms past presentation deadline)
    if (delta_us < -50000) {
        s->frame_queue.dropped_frames++;
        return true;
    }

    return false;
}

const char* bos_media_deadline_state_name(BOSMediaDeadlineState state) {
    switch (state) {
        case BOS_DEADLINE_EARLY:         return "EARLY";
        case BOS_DEADLINE_READY:         return "READY";
        case BOS_DEADLINE_LATE:          return "LATE";
        case BOS_DEADLINE_SEVERELY_LATE: return "SEVERELY_LATE";
        default:                         return "UNKNOWN";
    }
}
