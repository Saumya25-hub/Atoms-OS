#ifndef BCM_H
#define BCM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* BCM Error Codes                                                           */
/* ========================================================================= */
typedef enum {
    BCM_OK                  = 0,
    BCM_ERR_NULL_POINTER    = 1,
    BCM_ERR_INVALID_PARAM   = 2,
    BCM_ERR_QUEUE_FULL      = 3,
    BCM_ERR_BUSY            = 4,
    BCM_ERR_INVALID_STATE   = 5,
    BCM_ERR_NOT_INITIALIZED = 6,
    BCM_ERR_TIMEOUT         = 7
} bcm_error_t;

/* ========================================================================= */
/* BCM Frame States (Authoritative Frame Lifecycle State Machine)             */
/* ========================================================================= */
typedef enum {
    BCM_STATE_IDLE       = 0,
    BCM_STATE_REQUESTED  = 1,
    BCM_STATE_SCHEDULED  = 2,
    BCM_STATE_COMPOSING  = 3,
    BCM_STATE_COMPOSED   = 4,
    BCM_STATE_PRESENTING = 5,
    BCM_STATE_PRESENTED  = 6,
    BCM_STATE_ERROR      = 7
} BCM_FrameState;

/* ========================================================================= */
/* BCM Bounding Box / Damage Rectangle Structure                             */
/* ========================================================================= */
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} BCM_Rect;

#define BCM_MAX_DIRTY_RECTS 32

/* ========================================================================= */
/* BCM Performance & Reliability Telemetry Structure                         */
/* ========================================================================= */
typedef struct {
    uint64_t total_damage_requests;
    uint64_t coalesced_damage_requests;
    uint64_t full_repaint_count;
    uint64_t dropped_invalid_count;
    uint64_t irq_damage_requests;
    uint64_t task_damage_requests;
    uint64_t frames_requested;
    uint64_t frames_scheduled;
    uint64_t frames_composed;
    uint64_t frames_presented;
    uint64_t frames_coalesced;
    uint64_t frames_skipped;
    uint64_t missed_deadlines;
    uint32_t last_compose_time_us;
    uint32_t last_present_time_us;
    uint32_t max_compose_time_us;
    uint32_t max_present_time_us;
    uint32_t last_frame_duration_us;
    uint32_t worst_frame_duration_us;
    uint32_t average_frame_duration_us;
    uint32_t current_fps;
    uint32_t max_rect_count_observed;
    uint32_t last_damage_area;
    uint32_t dropped_frames;
    uint32_t reentrancy_blocks;
    uint32_t current_dirty_count;
    BCM_FrameState current_state;
} BCM_Telemetry;

/* ========================================================================= */
/* Public BCM API Contract                                                   */
/* ========================================================================= */

/**
 * @brief Initializes the BOS Composition Manager (BCM).
 *        Must be called during kernel initialization before desktop startup.
 * @return BCM_OK on success, error code otherwise.
 */
bcm_error_t BCM_Init(void);

/**
 * @brief IRQ-Safe Damage Request for an arbitrary screen bounding box.
 *        Guaranteed O(1), zero allocation, non-blocking, non-rendering.
 *        Merges overlapping or adjacent regions within bounded 32-rect envelope.
 */
void BCM_RequestDamage(int32_t x, int32_t y, int32_t width, int32_t height);

/**
 * @brief IRQ-Safe Damage Request for a specific BWE window.
 *        Queries window bounds and records damage without triggering rendering.
 */
void BCM_RequestWindowDamage(uint32_t window_id);

/**
 * @brief IRQ-Safe Damage Request for mouse cursor displacement.
 *        Records both the old and new 32x32 cursor bounding boxes.
 */
void BCM_RequestCursorDamage(int32_t old_x, int32_t old_y, int32_t new_x, int32_t new_y);

/**
 * @brief Request full-screen damage (e.g. on resolution or theme changes).
 */
void BCM_RequestFullRepaint(void);

/**
 * @brief IRQ-Safe Timer Tick Notification from hardware IRQ 0 (timer_tick_handler).
 *        Increments tick counter and updates frame pacer deadline without blocking.
 */
void BCM_NotifyTimerTick(uint64_t tick_count);

/**
 * @brief Core Compositor Execution Entry Point.
 *        MUST be called strictly from task context (RFLAGS.IF = 1), NEVER from an ISR.
 * @return BCM_OK on successful processing, BCM_ERR_BUSY if already active.
 */
bcm_error_t BCM_Process(void);

/**
 * @brief Dedicated Compositor Worker Thread Entry Point.
 */
void bcm_compositor_thread(void);

/**
 * @brief Spawn and start the dedicated BCM Compositor kernel task.
 */
bcm_error_t BCM_StartCompositorTask(void);

/**
 * @brief Get the current BCM frame state.
 */
BCM_FrameState BCM_GetState(void);

/**
 * @brief Query BCM runtime telemetry statistics.
 */
void BCM_GetTelemetry(BCM_Telemetry* out_telemetry);

/**
 * @brief Check if BCM has pending damage that requires composition.
 */
bool BCM_HasPendingDamage(void);

/**
 * @brief Check if frame pacing deadline has been reached (60 FPS target, ~16.6 ms).
 */
bool BCM_FrameDeadlineReached(void);

/**
 * @brief Query current count of active dirty rectangles.
 */
uint32_t BCM_GetDirtyRectCount(void);

/**
 * @brief Read-only access to current active dirty rectangle set.
 */
const BCM_Rect* BCM_GetDirtyRects(void);

/**
 * @brief Configure frame pacing interval in milliseconds (default 16ms for 60 FPS).
 */
void BCM_SetPacingInterval(uint32_t interval_ms);

/**
 * @brief Query frame pacing metrics.
 */
void BCM_GetPacingMetrics(uint32_t* out_fps, uint32_t* out_frame_time_us, uint32_t* out_missed_deadlines);

#ifdef __cplusplus
}
#endif

#endif /* BCM_H */
