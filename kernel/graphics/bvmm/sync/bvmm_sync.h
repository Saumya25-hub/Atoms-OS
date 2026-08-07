#ifndef _BOS_BVMM_SYNC_H_
#define _BOS_BVMM_SYNC_H_

#include "../lifetime/bvmm_lifetime.h"
#include "kernel/core/sync/spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_sync.h
 * @brief Production Fence & Synchronization Engine (BFSE V1.0) Master Header
 * 
 * BFSE is the permanent synchronization authority of the BOS GPU stack. It coordinates
 * execution ordering across CPU, GPU, BOCompositor, OpenGL, Vulkan, Video Decoders,
 * and multi-queue GPU engines without busy-waiting or global locks.
 */

#define BFSE_FENCE_CANARY_MAGIC     0x46454E43  /* "FENC" */
#define BFSE_TIMELINE_CANARY_MAGIC  0x54494D45  /* "TIME" */
#define BFSE_MAX_FENCES             8192
#define BFSE_MAX_TIMELINES          1024
#define BFSE_INVALID_FENCE_ID       0ULL
#define BFSE_INVALID_TIMELINE_ID    0ULL

typedef uint64_t bfse_fence_id_t;
typedef uint64_t bfse_timeline_id_t;

/* Fence Types (Phase 7A Spec) */
typedef enum {
    BFSE_FENCE_TIMELINE     = 0,
    BFSE_FENCE_BINARY       = 1,
    BFSE_FENCE_INTERRUPT    = 2,
    BFSE_FENCE_BARRIER      = 3
} bfse_fence_type_t;

/* Execution Queue Identifiers (Phase 7D Spec) */
typedef enum {
    BFSE_QUEUE_GRAPHICS     = 0,
    BFSE_QUEUE_COPY         = 1,
    BFSE_QUEUE_UPLOAD       = 2,
    BFSE_QUEUE_VIDEO_DECODE = 3,
    BFSE_QUEUE_COMPUTE      = 4
} bfse_queue_id_t;

/* Binary Fence States (Phase 7C Spec) */
typedef enum {
    BFSE_STATE_UNSIGNALED   = 0,
    BFSE_STATE_SIGNALED     = 1,
    BFSE_STATE_RESET        = 2
} bfse_binary_state_t;

/* Barrier Types (Phase 7H Spec) */
typedef enum {
    BFSE_BARRIER_EXECUTION  = 0,
    BFSE_BARRIER_MEMORY     = 1,
    BFSE_BARRIER_TEXTURE    = 2,
    BFSE_BARRIER_SURFACE    = 3,
    BFSE_BARRIER_UPLOAD     = 4,
    BFSE_BARRIER_READBACK   = 5,
    BFSE_BARRIER_SHADER     = 6
} bfse_barrier_type_t;

/* Monotonic Timeline Descriptor (Phase 7B Spec) */
typedef struct {
    uint32_t           canary_magic;   /* 0x54494D45 */
    bfse_timeline_id_t timeline_id;
    uint64_t           current_value;  /* Strictly monotonic */
    uint32_t           owner_pid;
    atoms_spinlock_t   lock;
} bfse_timeline_t;

/* Primary Fence Descriptor Structure */
typedef struct bfse_fence_desc {
    uint32_t           canary_magic;   /* 0x46454E43 */
    bfse_fence_id_t    fence_id;
    uint16_t           generation_id;
    uint32_t           registry_index;
    bfse_timeline_id_t timeline_id;
    uint64_t           target_timeline_value;
    bfse_fence_type_t  type;
    bfse_queue_id_t    queue_id;
    bfse_binary_state_t state;
    uint32_t           owner_pid;
    uint64_t           creation_time;
    uint64_t           signal_time;
    uint64_t           completion_time;
    uint32_t           wait_count;
    uint32_t           signal_count;
    uint32_t           dependency_count;
    uint32_t           ref_count;
} bfse_fence_desc_t;

/* Deadlock Report Descriptor (Phase 7I Spec) */
typedef struct {
    bool     deadlock_detected;
    uint32_t circular_wait_length;
    uint64_t stalled_fence_ids[16];
    uint32_t stalled_queue_ids[16];
} bfse_deadlock_report_t;

/* BFSE Diagnostics Summary */
typedef struct {
    uint32_t total_fences_created;
    uint32_t active_fences;
    uint32_t completed_fences;
    uint32_t pending_fences;
    uint32_t total_timelines;
    uint64_t total_fence_signals;
    uint64_t total_fence_waits;
    uint32_t deadlocks_detected;
    uint32_t barrier_count;
    uint32_t fence_reuse_count;
    uint32_t timeout_events;
    uint32_t validation_failures;
} bfse_diagnostics_t;

/**
 * @brief Initialize the BOS Fence & Synchronization Engine (BFSE).
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bfse_init(void);

/**
 * @brief Shutdown BFSE and release all registered synchronization objects.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bfse_shutdown(void);

/**
 * @brief Create a monotonic timeline fence object.
 * @param owner_pid Process ID of owner.
 * @param out_timeline_id Pointer to receive unique 64-bit Timeline ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bfse_timeline_create(uint32_t owner_pid, bfse_timeline_id_t* out_timeline_id);

/**
 * @brief Advance a monotonic timeline value.
 * @param timeline_id Target Timeline ID.
 * @param new_value New target value (must be > current value).
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bfse_timeline_advance(bfse_timeline_id_t timeline_id, uint64_t new_value);

/**
 * @brief Query current value of a monotonic timeline.
 * @param timeline_id Target Timeline ID.
 * @param out_value Pointer to receive current value.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bfse_timeline_query(bfse_timeline_id_t timeline_id, uint64_t* out_value);

/**
 * @brief Create a fence object tied to a timeline and queue.
 * @param timeline_id Backing Timeline ID.
 * @param target_value Timeline target value.
 * @param queue_id Queue identifier.
 * @param out_fence_id Pointer to receive unique 64-bit Fence ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bfse_fence_create(bfse_timeline_id_t timeline_id, uint64_t target_value, bfse_queue_id_t queue_id, bfse_fence_id_t* out_fence_id);

/**
 * @brief Destroy a fence object or return it to the recycling pool.
 * @param fence_id Target Fence ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bfse_fence_destroy(bfse_fence_id_t fence_id);

/**
 * @brief Signal a fence completion and advance associated timeline value.
 * @param fence_id Target Fence ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bfse_signal(bfse_fence_id_t fence_id);

/**
 * @brief Wait for a fence completion (non-blocking poll or timed wait).
 * @param fence_id Target Fence ID.
 * @param timeout_us Timeout in microseconds (0 for polling).
 * @return BVMM_SUCCESS if signaled, BVMM_ERR_OUT_OF_MEMORY if timed out/pending.
 */
bvmm_result_t bfse_wait(bfse_fence_id_t fence_id, uint64_t timeout_us);

/**
 * @brief Wait for ANY fence in an array to signal.
 * @param fence_ids Array of fence IDs.
 * @param count Number of fence IDs.
 * @param timeout_us Timeout in microseconds.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bfse_wait_any(const bfse_fence_id_t* fence_ids, uint32_t count, uint64_t timeout_us);

/**
 * @brief Wait for ALL fences in an array to signal.
 * @param fence_ids Array of fence IDs.
 * @param count Number of fence IDs.
 * @param timeout_us Timeout in microseconds.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bfse_wait_all(const bfse_fence_id_t* fence_ids, uint32_t count, uint64_t timeout_us);

/**
 * @brief Emit a pipeline barrier.
 * @param barrier_type Type of barrier.
 * @param queue_id Queue identifier.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bfse_barrier_emit(bfse_barrier_type_t barrier_type, bfse_queue_id_t queue_id);

/**
 * @brief Detect circular waits or deadlock stalls across GPU queues.
 * @param out_report Pointer to receive deadlock report.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bfse_detect_deadlocks(bfse_deadlock_report_t* out_report);

/**
 * @brief Dump developer inspection log for a specific fence.
 * @param fence_id Target Fence ID.
 */
void bfse_fence_dump(bfse_fence_id_t fence_id);

/**
 * @brief Dump all registered active fences.
 */
void bfse_fence_dump_all(void);

/**
 * @brief Retrieve current BFSE diagnostics summary.
 * @param out_diag Pointer to receive diagnostics descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bfse_get_diagnostics(bfse_diagnostics_t* out_diag);

/**
 * @brief Validate internal integrity of all registered synchronization objects.
 * @return true if valid, false if corruption or deadlock detected.
 */
bool bfse_validate_all(void);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_SYNC_H_ */
