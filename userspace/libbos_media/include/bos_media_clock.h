/*
 * ============================================================================
 * ATOMS OS — Monotonic Media Clock & Presentation Deadline Scheduler
 * userspace/libbos_media/include/bos_media_clock.h
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements:
 * - Realtime monotonic stream clock (media_time = current - start)
 * - Frame presentation deadline classifier (EARLY, READY, LATE, SEVERELY_LATE)
 * - Safe reference-aware frame drop evaluator
 * - Bounded Decoded Frame Queue (DFQ)
 * ============================================================================
 */

#ifndef BOS_MEDIA_CLOCK_H
#define BOS_MEDIA_CLOCK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOS_DEADLINE_EARLY         = 0,
    BOS_DEADLINE_READY         = 1,
    BOS_DEADLINE_LATE          = 2,
    BOS_DEADLINE_SEVERELY_LATE = 3
} BOSMediaDeadlineState;

typedef struct {
    uint64_t start_time_us;
    uint64_t paused_at_us;
    int64_t  base_offset_us;
    int64_t  observed_audio_clock_us;
    bool     is_paused;
    bool     is_running;
} BOSMediaClock;

typedef struct {
    uint32_t pic_id;
    int64_t  pts_us;
    int64_t  duration_us;
    bool     is_keyframe;
    bool     is_reference;
    uint8_t* yuv_data;
    uint32_t width;
    uint32_t height;
    uint32_t frame_crc;
} BOSDecodedFrame;

#define BOS_MAX_DECODED_FRAMES 4

typedef struct {
    BOSDecodedFrame frames[BOS_MAX_DECODED_FRAMES];
    uint32_t        read_idx;
    uint32_t        write_idx;
    uint32_t        count;
    uint32_t        high_water;
    uint32_t        low_water;
    uint64_t        dropped_frames;
    uint64_t        presented_frames;
} BOSFrameQueue;

typedef struct {
    BOSMediaClock  clock;
    BOSFrameQueue  frame_queue;
    int64_t        current_frame_deadline_delta_us;
    BOSMediaDeadlineState current_deadline_state;
} BOSMediaScheduler;

/* Clock Management */
void    bos_media_clock_init(BOSMediaClock* clock);
void    bos_media_clock_start(BOSMediaClock* clock, int64_t start_pts_us);
void    bos_media_clock_pause(BOSMediaClock* clock);
void    bos_media_clock_resume(BOSMediaClock* clock);
void    bos_media_clock_seek(BOSMediaClock* clock, int64_t new_pts_us);
int64_t bos_media_clock_get_time_us(BOSMediaClock* clock);
void    bos_media_clock_observe_audio(BOSMediaClock* clock, int64_t audio_pts_us);

/* Queue Management */
void    bos_frame_queue_init(BOSFrameQueue* q);
bool    bos_frame_queue_push(BOSFrameQueue* q, const BOSDecodedFrame* frame);
bool    bos_frame_queue_pop(BOSFrameQueue* q, BOSDecodedFrame* out_frame);
BOSDecodedFrame* bos_frame_queue_peek(BOSFrameQueue* q);
bool    bos_frame_queue_is_full(const BOSFrameQueue* q);
bool    bos_frame_queue_is_empty(const BOSFrameQueue* q);
void    bos_frame_queue_clear(BOSFrameQueue* q);

/* Scheduler Management */
void    bos_media_scheduler_init(BOSMediaScheduler* s);
void    bos_media_scheduler_start(BOSMediaScheduler* s, int64_t start_pts_us);
BOSMediaDeadlineState bos_media_scheduler_evaluate_deadline(BOSMediaScheduler* s, int64_t frame_pts_us, int64_t* out_delta_us);
bool    bos_media_scheduler_should_drop_frame(BOSMediaScheduler* s, const BOSDecodedFrame* frame, int64_t delta_us);
const char* bos_media_deadline_state_name(BOSMediaDeadlineState state);

#ifdef __cplusplus
}
#endif

#endif /* BOS_MEDIA_CLOCK_H */
