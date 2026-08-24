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
    BCM_ERR_TIMEOUT         = 7,
    BCM_ERR_BACKEND_FAILED  = 8,
    BCM_ERR_STALE_FRAME     = 9
} bcm_error_t;

/* ========================================================================= */
/* BCM Frame States (Authoritative Frame Lifecycle State Machine)             */
/* ========================================================================= */
typedef enum {
    BCM_STATE_IDLE              = 0,
    BCM_STATE_REQUESTED         = 1,
    BCM_STATE_SCHEDULED         = 2,
    BCM_STATE_COMPOSING         = 3,
    BCM_STATE_COMPOSED          = 4,
    BCM_STATE_PRESENT_QUEUED    = 5,
    BCM_STATE_PRESENTING        = 6,
    BCM_STATE_PRESENT_COMPLETE  = 7,
    BCM_STATE_PRESENTED         = 8,
    BCM_STATE_ERROR             = 9
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
    
    /* Phase 6 & Phase 7 Presentation & Completion Telemetry */
    uint64_t presentation_requests;
    uint64_t presentation_submissions;
    uint64_t presentation_completions;
    uint64_t presentation_failures;
    uint64_t presentation_timeouts;
    uint64_t dropped_presentations;
    uint64_t current_frame_id;
    uint64_t last_completed_frame_id;
    uint64_t in_flight_frame_id;
    bool     is_in_flight;

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

/* ========================================================================= */
/* Phase 6 & Phase 7 Presentation & Synchronization APIs                     */
/* ========================================================================= */

/**
 * @brief Enqueue a composed frame for presentation.
 * @param frame_id Unique identifier of the composed frame.
 * @return BCM_OK on success, error code otherwise.
 */
bcm_error_t BCM_SchedulePresentation(uint64_t frame_id);

/**
 * @brief Transition frame into active presentation (In-Flight lock acquired).
 * @param frame_id Unique identifier of the frame to present.
 * @return BCM_OK on success, BCM_ERR_BUSY if another frame is already in flight.
 */
bcm_error_t BCM_BeginPresentation(uint64_t frame_id);

/**
 * @brief Complete presentation and retire frame resources.
 * @param frame_id Unique identifier of the presenting frame.
 * @param status Backend execution result (BCM_OK or error).
 * @return BCM_OK on success, error code if stale or unknown frame.
 */
bcm_error_t BCM_CompletePresentation(uint64_t frame_id, bcm_error_t status);

/**
 * @brief Check if any frame is currently in-flight in presentation.
 */
bool BCM_IsFrameInFlight(void);

/**
 * @brief Query current in-flight frame ID (0 if none).
 */
uint64_t BCM_GetInFlightFrameID(void);

/**
 * @brief Query current monotonically assigned frame ID.
 */
uint64_t BCM_GetCurrentFrameID(void);

/**
 * @brief Query last successfully completed and retired frame ID.
 */
uint64_t BCM_GetLastCompletedFrameID(void);

/**
 * @brief Check for presentation timeout and perform controlled recovery if exceeded.
 * @param timeout_ms Maximum allowed presentation duration in milliseconds.
 * @return BCM_OK if healthy, BCM_ERR_TIMEOUT if timed out and recovered.
 */
bcm_error_t BCM_CheckPresentationTimeout(uint64_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* BCM_H */
