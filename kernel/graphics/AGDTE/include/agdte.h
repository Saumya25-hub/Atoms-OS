#ifndef ATOMS_OS_AGDTE_H
#define ATOMS_OS_AGDTE_H

/**
 * @file agdte.h
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Master Public Header
 * @status Phase 2 Core Architecture & Display Scheduler Frozen
 *
 * @section PHILOSOPHY
 * ATOMEGearDisplayTrainEngine is NOT another renderer. It is NOT another compositor.
 * It is NOT another graphics library. It is the operating system's master display controller.
 * Every rendered frame must pass through AGDTE before reaching the monitor.
 *
 * @section PERFORMANCE_RULES
 * - Zero heap allocations (All data structures static/fixed-pool managed).
 * - Zero floating point arithmetic (Deterministic integer & fixed-point microsecond/tick math).
 * - Zero blocking loops (Event/scheduler-driven deterministic transitions).
 * - Zero unnecessary memory copies (Pure ownership handoff).
 * - Zero duplicate frame ownership (Single authoritative buffer ownership model).
 */

#include <stdint.h>
#include <stdbool.h>
#include "../../BSPE/include/bspe.h"
#include "../../BOGE/include/boge.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Version & Identification --- */
#define AGDTE_VERSION_MAJOR         2
#define AGDTE_VERSION_MINOR         0
#define AGDTE_VERSION_PATCH         0
#define AGDTE_MAGIC_HEADER          0x41474454 /* 'AGDT' */

/* --- Capacity & Pool Limits (Static Memory Model) --- */
#define AGDTE_MAX_DISPLAYS          4
#define AGDTE_MAX_QUEUE_DEPTH       32
#define AGDTE_MAX_MANAGED_SURFACES  64
#define AGDTE_MAX_BUFFERS           64
#define AGDTE_MAX_DIRTY_RECTS       32
#define AGDTE_MAX_TIMING_HISTORY    16

/* --- Error Return Codes --- */
typedef enum {
    AGDTE_OK                    =  0,
    AGDTE_ERR_NULL_POINTER      = -1,
    AGDTE_ERR_OUT_OF_MEMORY     = -2,
    AGDTE_ERR_INVALID_STATE     = -3,
    AGDTE_ERR_QUEUE_FULL        = -4,
    AGDTE_ERR_QUEUE_EMPTY       = -5,
    AGDTE_ERR_INVALID_SURFACE   = -6,
    AGDTE_ERR_INVALID_BUFFER    = -7,
    AGDTE_ERR_INVALID_DISPLAY   = -8,
    AGDTE_ERR_BACKEND_FAILED    = -9,
    AGDTE_ERR_OWNERSHIP_VIOLATION = -10,
    AGDTE_ERR_TIMEOUT           = -11,
    AGDTE_ERR_UNSUPPORTED       = -12
} AGDTE_Error;

/* --- Priority Levels for Presentation Requests --- */
typedef enum {
    AGDTE_PRIORITY_LOW          = 0,
    AGDTE_PRIORITY_NORMAL       = 1,
    AGDTE_PRIORITY_HIGH         = 2,
    AGDTE_PRIORITY_CRITICAL_CURSOR = 3,
    AGDTE_PRIORITY_REALTIME_SYNC   = 4
} AGDTE_Priority;

/* --- Scheduler Decision Verdicts --- */
typedef enum {
    AGDTE_DECISION_NONE         = 0,
    AGDTE_DECISION_PRESENT_NOW  = 1,
    AGDTE_DECISION_WAIT_PACING  = 2,
    AGDTE_DECISION_SKIP_SUPERSEDED = 3,
    AGDTE_DECISION_MERGE_BATCH  = 4,
    AGDTE_DECISION_FORCE_PRESENT = 5
} AGDTE_SchedulerDecision;

/* --- Surface Layer Classifications --- */
typedef enum {
    AGDTE_LAYER_DESKTOP         = 0,
    AGDTE_LAYER_WINDOWS         = 1,
    AGDTE_LAYER_CURSOR          = 2,
    AGDTE_LAYER_POPUP           = 3,
    AGDTE_LAYER_OVERLAY         = 4,
    AGDTE_LAYER_NOTIFICATION    = 5,
    AGDTE_LAYER_VIDEO_FUTURE    = 6,
    AGDTE_LAYER_HW_CURSOR_FUTURE = 7,
    AGDTE_LAYER_HDR_FUTURE      = 8,
    AGDTE_LAYER_COUNT           = 9
} AGDTE_SurfaceLayer;

/* --- Buffer Ownership States --- */
typedef enum {
    AGDTE_BUFFER_OWNER_NONE         = 0,
    AGDTE_BUFFER_OWNER_BOGE_RENDER  = 1,
    AGDTE_BUFFER_OWNER_BSPE_STAGING = 2,
    AGDTE_BUFFER_OWNER_AGDTE_QUEUE  = 3,
    AGDTE_BUFFER_OWNER_DISPLAY_ACTIVE = 4
} AGDTE_BufferOwner;

/* --- Buffer Roles --- */
typedef enum {
    AGDTE_BUFFER_ROLE_BACK          = 0,
    AGDTE_BUFFER_ROLE_FRONT         = 1,
    AGDTE_BUFFER_ROLE_STAGING       = 2,
    AGDTE_BUFFER_ROLE_TEMP          = 3,
    AGDTE_BUFFER_ROLE_DIRTY_MASK    = 4
} AGDTE_BufferRole;

/* --- Hardware Backend Identifiers --- */
typedef enum {
    AGDTE_BACKEND_NONE          = 0,
    AGDTE_BACKEND_VBE           = 1,
    AGDTE_BACKEND_VMWARE_SVGA   = 2,
    AGDTE_BACKEND_VIRTIO_GPU    = 3,
    AGDTE_BACKEND_INTEL_FUTURE  = 4,
    AGDTE_BACKEND_AMD_FUTURE    = 5,
    AGDTE_BACKEND_NVIDIA_FUTURE = 6
} AGDTE_BackendType;

/* --- Display Cadence / Sync Modes --- */
typedef enum {
    AGDTE_CADENCE_IMMEDIATE     = 0, /* Tearing allowed / minimum latency test */
    AGDTE_CADENCE_60HZ_FIXED    = 1, /* 16.666 ms period */
    AGDTE_CADENCE_144HZ_FIXED   = 2, /* 6.944 ms period */
    AGDTE_CADENCE_ADAPTIVE_SYNC = 3, /* Future VRR/G-Sync/FreeSync */
    AGDTE_CADENCE_VSYNC_IRQ     = 4  /* Future hardware VBI interrupt driven */
} AGDTE_CadenceMode;

/* --- Core Data Structures --- */

/**
 * @struct AGDTE_TimingRecord
 * @brief Deterministic timing and cadence metrics for a presentation request.
 */
typedef struct {
    uint64_t frame_id;
    uint64_t submit_timestamp_us;
    uint64_t scheduled_timestamp_us;
    uint64_t presentation_timestamp_us;
    uint64_t target_deadline_us;
    uint32_t cadence_period_us;
    bool vsync_aligned;
} AGDTE_TimingRecord;

/**
 * @struct AGDTE_PresentRequest
 * @brief Represents a single presentation submission queued into AGDTE.
 */
typedef struct {
    uint32_t request_id;
    uint32_t frame_id;
    uint32_t display_id;
    uint32_t buffer_id;
    AGDTE_Priority priority;
    AGDTE_SurfaceLayer target_layer;
    uint64_t submit_time_us;
    uint64_t target_deadline_us;
    BOGE_Rect dirty_rects[AGDTE_MAX_DIRTY_RECTS];
    uint32_t dirty_count;
    bool allow_skip;
    bool force_immediate;
    bool cancelled;
} AGDTE_PresentRequest;

/**
 * @struct AGDTE_SurfaceDescriptor
 * @brief Managed surface entry within the AGDTE Surface Train.
 */
typedef struct {
    uint32_t surface_id;
    uint32_t display_id;
    AGDTE_SurfaceLayer layer;
    int32_t pos_x;
    int32_t pos_y;
    uint32_t width;
    uint32_t height;
    uint32_t current_buffer_id;
    bool visible;
    bool opaque;
    uint32_t z_index;
    char name[32];
} AGDTE_SurfaceDescriptor;

/**
 * @struct AGDTE_BufferDescriptor
 * @brief Authoritative buffer metadata and strict ownership tracking.
 */
typedef struct {
    uint32_t buffer_id;
    void* virtual_address;
    uint64_t physical_address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    AGDTE_BufferRole role;
    AGDTE_BufferOwner owner;
    uint32_t assigned_surface_id;
    bool is_locked;
} AGDTE_BufferDescriptor;

/**
 * @struct AGDTE_DisplayState
 * @brief Physical and logical state of an attached display device.
 */
typedef struct {
    uint32_t display_id;
    bool active;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch_bytes;
    AGDTE_BackendType backend_type;
    AGDTE_CadenceMode cadence_mode;
    uint32_t refresh_rate_hz;
    uint64_t last_vbi_timestamp_us;
    uint32_t active_frontbuffer_id;
    uint32_t active_backbuffer_id;
} AGDTE_DisplayState;

/**
 * @struct AGDTE_Diagnostics
 * @brief High-precision telemetry metrics for the entire AGDTE subsystem.
 */
typedef struct {
    uint64_t frames_submitted;
    uint64_t frames_scheduled;
    uint64_t frames_presented;
    uint64_t frames_skipped;
    uint64_t frames_delayed;
    uint64_t frames_batched;
    uint32_t queue_usage_current;
    uint32_t queue_depth_max;
    uint32_t queue_depth_avg_x100;
    uint64_t scheduler_decisions[6]; /* Track counts per AGDTE_SchedulerDecision enum */
    uint64_t present_time_last_us;
    uint64_t present_time_worst_us;
    uint64_t present_time_avg_us;
    uint64_t pulse_time_last_us;
    uint64_t pulse_time_worst_us;
    uint64_t cursor_present_time_us;
    uint64_t window_present_time_us;
    uint32_t surface_count;
    uint64_t buffer_copies;
    uint64_t dirty_rect_merge_count;
    AGDTE_BackendType active_backend;
    uint64_t presentation_latency_us;
} AGDTE_Diagnostics;

/**
 * @struct AGDTE_BackendOps
 * @brief Hardware abstraction interface table for display/gpu backends.
 */
typedef struct {
    AGDTE_Error (*init)(uint32_t display_id, uint32_t width, uint32_t height, uint32_t bpp);
    AGDTE_Error (*set_mode)(uint32_t display_id, uint32_t width, uint32_t height, uint32_t bpp);
    AGDTE_Error (*present_buffer)(uint32_t display_id, const AGDTE_BufferDescriptor* buffer, const BOGE_Rect* dirty_rects, uint32_t dirty_count);
    AGDTE_Error (*flip_page)(uint32_t display_id, uint32_t buffer_id);
    AGDTE_Error (*query_vsync)(uint32_t display_id, bool* out_vbi_active, uint64_t* out_timestamp_us);
    AGDTE_Error (*set_cursor_pos)(uint32_t display_id, int32_t x, int32_t y);
    void        (*shutdown)(uint32_t display_id);
} AGDTE_BackendOps;

/* --- Subsystem Public API Functions --- */

/* Core Lifecycle (agdte.c) */
AGDTE_Error AGDTE_Initialize(void);
void        AGDTE_Shutdown(void);
bool        AGDTE_IsInitialized(void);
AGDTE_Error AGDTE_Pulse(uint64_t current_time_us);

/* Display Scheduler (agdte_scheduler.c) */
AGDTE_SchedulerDecision AGDTE_Scheduler_Evaluate(const AGDTE_PresentRequest* req, uint64_t current_time_us);
AGDTE_Error             AGDTE_Scheduler_MergeDirtyRegions(AGDTE_PresentRequest* target, const AGDTE_PresentRequest* source);
uint64_t                AGDTE_Scheduler_GetNextScheduledTimeUs(void);

/* Presentation Queue (agdte_present_queue.c) */
AGDTE_Error AGDTE_Queue_Submit(const AGDTE_PresentRequest* req, uint32_t* out_request_id);
AGDTE_Error AGDTE_Queue_PopNext(AGDTE_PresentRequest* out_req);
AGDTE_Error AGDTE_Queue_PeekNext(AGDTE_PresentRequest* out_req);
AGDTE_Error AGDTE_Queue_CancelRequest(uint32_t request_id);
AGDTE_Error AGDTE_Queue_Flush(void);
uint32_t    AGDTE_Queue_GetDepth(void);

/* Surface Manager (agdte_surface_manager.c) */
AGDTE_Error AGDTE_Surface_Register(uint32_t display_id, AGDTE_SurfaceLayer layer, uint32_t width, uint32_t height, const char* name, uint32_t* out_surface_id);
AGDTE_Error AGDTE_Surface_Unregister(uint32_t surface_id);
AGDTE_Error AGDTE_Surface_SetPosition(uint32_t surface_id, int32_t x, int32_t y);
AGDTE_Error AGDTE_Surface_AssignBuffer(uint32_t surface_id, uint32_t buffer_id);
AGDTE_SurfaceDescriptor* AGDTE_Surface_GetDescriptor(uint32_t surface_id);
AGDTE_Error              AGDTE_Surface_GetLayerSurfaceID(AGDTE_SurfaceLayer layer, uint32_t* out_id);
uint32_t                 AGDTE_Surface_GetActiveCount(void);

/* Buffer Manager (agdte_buffer_manager.c) */
AGDTE_Error AGDTE_Buffer_Register(void* virtual_address, uint32_t width, uint32_t height, uint32_t pitch, AGDTE_BufferRole role, uint32_t* out_buffer_id);
AGDTE_Error AGDTE_Buffer_Unregister(uint32_t buffer_id);
AGDTE_Error AGDTE_Buffer_TransferOwnership(uint32_t buffer_id, AGDTE_BufferOwner new_owner);
AGDTE_BufferDescriptor* AGDTE_Buffer_GetDescriptor(uint32_t buffer_id);

/* Display State (agdte_display_state.c) */
AGDTE_Error AGDTE_Display_Register(uint32_t width, uint32_t height, uint32_t bpp, AGDTE_BackendType backend, uint32_t* out_display_id);
AGDTE_Error AGDTE_Display_SetCadenceMode(uint32_t display_id, AGDTE_CadenceMode mode, uint32_t refresh_hz);
AGDTE_DisplayState* AGDTE_Display_GetState(uint32_t display_id);

/* Timing Engine (agdte_timing.c) */
AGDTE_Error AGDTE_Timing_RecordSubmit(uint32_t request_id, uint64_t submit_time_us, uint64_t target_deadline_us);
AGDTE_Error AGDTE_Timing_RecordPresentation(uint32_t request_id, uint64_t present_time_us);
uint64_t    AGDTE_Timing_CalculateCadenceDeadline(uint32_t display_id, uint64_t current_time_us);

/* Presenter Engine (agdte_presenter.c) */
AGDTE_Error AGDTE_Presenter_Execute(const AGDTE_PresentRequest* req, uint64_t current_time_us);
AGDTE_Error AGDTE_Presenter_PresentBridgeBSPE(const BOGE_StagingFrame* boge_frame, uint32_t display_id);

/* Diagnostics Engine (agdte_diag.c) */
void               AGDTE_Diag_Init(void);
void               AGDTE_Diag_RecordDecision(AGDTE_SchedulerDecision decision);
void               AGDTE_Diag_RecordQueueDepth(uint32_t current_depth);
void               AGDTE_Diag_RecordLatency(uint64_t duration_us);
void               AGDTE_Diag_RecordPulseTime(uint64_t duration_us);
void               AGDTE_Diag_RecordLayerPresentTime(AGDTE_SurfaceLayer layer, uint64_t duration_us);
void               AGDTE_Diag_UpdateSurfaceCount(uint32_t count);
AGDTE_Diagnostics* AGDTE_Diag_GetSnapshot(void);
void               AGDTE_Diag_DumpConsole(void);

/* Backend Layer (agdte_backend.c) */
AGDTE_Error AGDTE_Backend_Register(AGDTE_BackendType type, const AGDTE_BackendOps* ops);
AGDTE_Error AGDTE_Backend_SetCurrent(uint32_t display_id, AGDTE_BackendType type);
const AGDTE_BackendOps* AGDTE_Backend_GetOps(AGDTE_BackendType type);

/* =========================================================================
 * PHASE 4 DISPLAY TIMING OPTIMIZATION LAYER
 * Presentation Timeline, Frame Metrics, VSync, Frame Pacer, Refresh & Swap
 * ========================================================================= */

/* --- Presentation Timeline (agdte_present_timeline.c) --- */
typedef struct {
    uint64_t frame_id;
    uint64_t submit_timestamp_us;
    uint64_t queue_timestamp_us;
    uint64_t schedule_timestamp_us;
    uint64_t present_timestamp_us;
    uint64_t display_timestamp_us;
    uint64_t completion_timestamp_us;
    uint32_t display_id;
    AGDTE_SurfaceLayer layer;
    bool valid;
} AGDTE_TimelineEntry;

AGDTE_Error          AGDTE_Timeline_Init(void);
AGDTE_Error          AGDTE_Timeline_RecordSubmit(uint64_t frame_id, uint32_t display_id, AGDTE_SurfaceLayer layer, uint64_t timestamp_us);
AGDTE_Error          AGDTE_Timeline_RecordQueue(uint64_t frame_id, uint64_t timestamp_us);
AGDTE_Error          AGDTE_Timeline_RecordSchedule(uint64_t frame_id, uint64_t timestamp_us);
AGDTE_Error          AGDTE_Timeline_RecordPresent(uint64_t frame_id, uint64_t timestamp_us);
AGDTE_Error          AGDTE_Timeline_RecordDisplay(uint64_t frame_id, uint64_t timestamp_us);
AGDTE_Error          AGDTE_Timeline_RecordCompletion(uint64_t frame_id, uint64_t timestamp_us);
AGDTE_TimelineEntry* AGDTE_Timeline_GetEntry(uint64_t frame_id);
AGDTE_TimelineEntry* AGDTE_Timeline_GetLatest(void);

/* --- Frame Metrics (agdte_frame_metrics.c) --- */
typedef struct {
    uint64_t avg_frame_time_us;
    uint64_t worst_frame_time_us;
    uint64_t presentation_latency_us;
    uint64_t queue_delay_us;
    uint64_t scheduler_delay_us;
    uint64_t swap_delay_us;
    uint64_t display_delay_us;
    uint64_t dropped_frames;
    uint64_t skipped_frames;
    uint64_t late_frames;
    uint64_t early_frames;
    uint64_t burst_frames;
    uint64_t frame_variance_us;
    uint64_t presentation_variance_us;
    uint64_t total_frames_measured;
} AGDTE_FrameMetricsSnapshot;

void                        AGDTE_Metrics_Init(void);
void                        AGDTE_Metrics_OnFrameComplete(const AGDTE_TimelineEntry* entry, uint64_t target_interval_us);
void                        AGDTE_Metrics_RecordDropped(void);
void                        AGDTE_Metrics_RecordSkipped(void);
AGDTE_FrameMetricsSnapshot* AGDTE_Metrics_GetSnapshot(void);

/* --- VSync Abstraction Layer (agdte_vsync.c) --- */
typedef enum {
    AGDTE_VSYNC_STATE_IDLE       = 0,
    AGDTE_VSYNC_STATE_REQUESTED  = 1,
    AGDTE_VSYNC_STATE_ACTIVE     = 2,
    AGDTE_VSYNC_STATE_TRIGGERED  = 3
} AGDTE_VSyncStatus;

typedef struct {
    uint32_t          display_id;
    AGDTE_VSyncStatus status;
    uint64_t          requested_target_us;
    uint64_t          last_vbi_timestamp_us;
    uint64_t          vbi_interval_us;
    uint32_t          irq_trigger_count;
    bool              enabled;
} AGDTE_VSyncState;

AGDTE_Error       AGDTE_VSync_Init(uint32_t display_id);
AGDTE_Error       AGDTE_VSync_RequestSync(uint32_t display_id, uint64_t target_time_us);
AGDTE_Error       AGDTE_VSync_OnVBI_IRQ(uint32_t display_id, uint64_t irq_timestamp_us);
AGDTE_Error       AGDTE_VSync_Poll(uint32_t display_id, uint64_t current_time_us);
bool              AGDTE_VSync_IsSyncReady(uint32_t display_id, uint64_t current_time_us);
AGDTE_VSyncState* AGDTE_VSync_GetState(uint32_t display_id);

/* --- Deterministic Frame Pacer (agdte_frame_pacer.c) --- */
typedef enum {
    AGDTE_PACER_READY  = 0, /* Target presentation deadline reached/aligned -> Present now */
    AGDTE_PACER_WAIT   = 1, /* Too early -> Retain in queue */
    AGDTE_PACER_BURST  = 2, /* Burst detected -> Rate limit / pace */
    AGDTE_PACER_LATE   = 3  /* Deadline passed -> Present immediately and resync */
} AGDTE_PacerVerdict;

typedef struct {
    uint32_t display_id;
    uint32_t target_rate_hz;
    uint64_t target_interval_us;
    uint64_t last_presentation_us;
    uint64_t next_presentation_deadline_us;
    uint64_t jitter_tolerance_us;
    bool     active;
} AGDTE_FramePacerState;

AGDTE_Error            AGDTE_Pacer_Init(uint32_t display_id, uint32_t target_refresh_hz);
AGDTE_Error            AGDTE_Pacer_SetRate(uint32_t display_id, uint32_t target_refresh_hz);
uint64_t               AGDTE_Pacer_CalculateNextDeadline(uint32_t display_id, uint64_t current_time_us);
AGDTE_PacerVerdict     AGDTE_Pacer_EvaluateReadiness(uint32_t display_id, uint64_t current_time_us, uint64_t request_deadline_us);
AGDTE_Error            AGDTE_Pacer_OnPresentationExecuted(uint32_t display_id, uint64_t present_time_us);
AGDTE_FramePacerState* AGDTE_Pacer_GetState(uint32_t display_id);

/* --- Refresh Controller (agdte_refresh_controller.c) --- */
typedef struct {
    uint32_t          display_id;
    uint32_t          current_refresh_hz;
    AGDTE_CadenceMode cadence_mode;
    uint64_t          refresh_interval_us;
    bool              synchronized;
} AGDTE_RefreshProfile;

AGDTE_Error           AGDTE_RefreshController_Init(void);
AGDTE_Error           AGDTE_RefreshController_SetProfile(uint32_t display_id, uint32_t refresh_hz, AGDTE_CadenceMode mode);
uint64_t              AGDTE_RefreshController_GetIntervalUs(uint32_t display_id);
AGDTE_Error           AGDTE_RefreshController_SynchronizeTimers(uint64_t current_time_us);
AGDTE_RefreshProfile* AGDTE_RefreshController_GetProfile(uint32_t display_id);

/* --- Swap Controller & Triple Buffering (agdte_swap_controller.c) --- */
typedef enum {
    AGDTE_SWAP_DECISION_COMMIT   = 0,
    AGDTE_SWAP_DECISION_HOLD     = 1,
    AGDTE_SWAP_DECISION_DISCARD  = 2
} AGDTE_SwapDecision;

typedef struct {
    uint32_t display_id;
    uint32_t front_buffer_id;   /* Scanning out on hardware monitor */
    uint32_t back_buffer_id;    /* Staging / ready for next page flip */
    uint32_t pending_buffer_id; /* Enqueued buffer awaiting VSync/Pacer deadline */
    uint32_t pending_request_id;
    uint64_t pending_deadline_us;
    bool     swap_pending;
    uint64_t last_swap_time_us;
    uint64_t total_swaps_executed;
    uint64_t duplicate_swaps_prevented;
} AGDTE_SwapControllerState;

AGDTE_Error                AGDTE_SwapController_Init(uint32_t display_id);
AGDTE_Error                AGDTE_SwapController_SubmitBuffer(uint32_t display_id, uint32_t buffer_id, uint32_t request_id, uint64_t deadline_us);
AGDTE_SwapDecision         AGDTE_SwapController_EvaluateSwap(uint32_t display_id, uint64_t current_time_us, uint32_t* out_buffer_id, uint32_t* out_request_id);
AGDTE_Error                AGDTE_SwapController_CommitSwap(uint32_t display_id, uint32_t buffer_id, uint64_t present_time_us);
AGDTE_SwapControllerState* AGDTE_SwapController_GetState(uint32_t display_id);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_AGDTE_H */
