#ifndef _BOS_BVMM_LIFETIME_H_
#define _BOS_BVMM_LIFETIME_H_

#include "../texture/bvmm_texture.h"
#include "../surface/bvmm_surface.h"
#include "kernel/core/sync/spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_lifetime.h
 * @brief Production Reference, Residency & Lifetime Engine (BRRLE V1.0) Master Header
 * 
 * BRRLE is the permanent GPU resource lifetime authority of BOS. It synchronizes
 * CPU/GPU reference counters, validates residency state machine transitions,
 * enforces resource pinning, tracks migration metrics, and integrates timeline fences.
 */

#define BRRLE_LIFETIME_CANARY_MAGIC 0x4C494645  /* "LIFE" */
#define BRRLE_MAX_LIFETIMES         8192
#define BRRLE_INVALID_LIFETIME_ID   0ULL

typedef uint64_t brrle_lifetime_id_t;

/* GPU Resource Types (Phase 6A Spec) */
typedef enum {
    BRRLE_RES_TEXTURE         = 0,
    BRRLE_RES_SURFACE         = 1,
    BRRLE_RES_BUFFER          = 2,
    BRRLE_RES_RENDER_TARGET   = 3,
    BRRLE_RES_SHADER_RESOURCE = 4,
    BRRLE_RES_VIDEO_SURFACE   = 5
} brrle_resource_type_t;

/* Residency Locations (Phase 6C Spec) */
typedef enum {
    BRRLE_RESIDENCY_VRAM          = 0,
    BRRLE_RESIDENCY_GTT           = 1,
    BRRLE_RESIDENCY_SYSTEM_MEMORY = 2,
    BRRLE_RESIDENCY_UPLOAD        = 3,
    BRRLE_RESIDENCY_READBACK      = 4,
    BRRLE_RESIDENCY_EVICTED       = 5
} brrle_residency_t;

/* Residency Lifetime State Machine (Phase 6D Spec) */
typedef enum {
    BRRLE_STATE_CREATED           = 0,
    BRRLE_STATE_RESIDENT          = 1,
    BRRLE_STATE_MAPPED            = 2,
    BRRLE_STATE_REFERENCED        = 3,
    BRRLE_STATE_IDLE              = 4,
    BRRLE_STATE_MIGRATION_PENDING = 5,
    BRRLE_STATE_MIGRATING         = 6,
    BRRLE_STATE_EVICTED           = 7,
    BRRLE_STATE_DESTROYED         = 8
} brrle_state_t;

/* Resource Pinning Modes (Phase 6E Spec) */
typedef enum {
    BRRLE_PIN_UNPINNED            = 0,
    BRRLE_PIN_PINNED              = 1,
    BRRLE_PIN_TEMPORARY           = 2,
    BRRLE_PIN_SCANOUT             = 3,
    BRRLE_PIN_CURSOR              = 4,
    BRRLE_PIN_SYSTEM              = 5
} brrle_pin_state_t;

/* Resource Priority Classes (Phase 6H Spec) */
typedef enum {
    BRRLE_PRIORITY_BACKGROUND     = 0,
    BRRLE_PRIORITY_STREAMING      = 1,
    BRRLE_PRIORITY_LOW            = 2,
    BRRLE_PRIORITY_NORMAL         = 3,
    BRRLE_PRIORITY_HIGH           = 4,
    BRRLE_PRIORITY_CRITICAL       = 5
} brrle_priority_t;

/* Timeline Fence Synchronization Descriptor (Phase 6F Spec) */
typedef struct {
    uint64_t fence_id;
    uint16_t fence_generation;
    uint32_t wait_count;
    uint64_t signal_timestamp;
    bool     is_signaled;
} brrle_fence_t;

/* Migration Tracking Information (Phase 6G Spec) */
typedef struct {
    brrle_residency_t current_residency;
    brrle_residency_t target_residency;
    uint32_t          migration_reason;
    uint64_t          migration_timestamp;
    uint32_t          migration_count;
    uint64_t          migration_latency_us;
    bool              is_migrating;
} brrle_migration_info_t;

/* Creation Info Structure */
typedef struct {
    brrle_resource_type_t resource_type;
    btfe_texture_id_t     texture_id;
    bvmm_surface_id_t     surface_id;
    brrle_residency_t     initial_residency;
    brrle_priority_t      priority;
    uint32_t              owner_pid;
} brrle_lifetime_create_info_t;

/* Primary Resource Lifetime Descriptor Structure */
typedef struct brrle_lifetime_desc {
    uint32_t              canary_magic;    /* 0x4C494645 */
    brrle_lifetime_id_t   lifetime_id;
    uint16_t              generation_id;
    uint32_t              registry_index;
    brrle_resource_type_t resource_type;
    btfe_texture_id_t     texture_id;
    bvmm_surface_id_t     surface_id;
    uint32_t              owner_pid;
    uint64_t              creation_time;
    uint64_t              last_access_time;
    uint32_t              cpu_refcount;
    uint32_t              gpu_refcount;
    brrle_fence_t         fence;
    brrle_state_t         state;
    brrle_residency_t     residency;
    brrle_priority_t      priority;
    brrle_pin_state_t     pin_state;
    brrle_migration_info_t migration;
} brrle_lifetime_desc_t;

/* BRRLE Diagnostics Summary */
typedef struct {
    uint32_t              total_resources_tracked;
    uint32_t              resident_resources;
    uint32_t              pinned_resources;
    uint32_t              evicted_resources;
    uint32_t              zombie_resources_detected;
    uint32_t              fence_wait_count;
    uint32_t              migration_count_total;
    uint32_t              residency_histogram[6];
    uint32_t              priority_histogram[6];
    uint32_t              ref_leaks_detected;
    uint32_t              validation_failures;
} brrle_diagnostics_t;

/**
 * @brief Initialize the Production Reference, Residency & Lifetime Engine (BRRLE).
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_init(void);

/**
 * @brief Shutdown BRRLE and destroy all tracked lifetime descriptors.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_shutdown(void);

/**
 * @brief Create a lifetime descriptor for a GPU resource.
 * @param info Creation descriptor.
 * @param out_lifetime_id Pointer to receive unique 64-bit Lifetime ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_lifetime_create(const brrle_lifetime_create_info_t* info, brrle_lifetime_id_t* out_lifetime_id);

/**
 * @brief Destroy a lifetime descriptor and trigger backing resource destruction if refcounts permit.
 * @param lifetime_id Target Lifetime ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_lifetime_destroy(brrle_lifetime_id_t lifetime_id);

/**
 * @brief Lookup a lifetime descriptor by Lifetime ID.
 * @param lifetime_id Target Lifetime ID.
 * @param out_desc Pointer to receive descriptor pointer.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_lifetime_lookup(brrle_lifetime_id_t lifetime_id, brrle_lifetime_desc_t** out_desc);

/**
 * @brief Increment CPU reference count.
 * @param lifetime_id Target Lifetime ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_cpu_acquire(brrle_lifetime_id_t lifetime_id);

/**
 * @brief Decrement CPU reference count. Triggers auto-destruction if CPU==0 and GPU==0 and Fences completed.
 * @param lifetime_id Target Lifetime ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_cpu_release(brrle_lifetime_id_t lifetime_id);

/**
 * @brief Increment GPU reference count.
 * @param lifetime_id Target Lifetime ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_gpu_acquire(brrle_lifetime_id_t lifetime_id);

/**
 * @brief Decrement GPU reference count. Triggers auto-destruction if CPU==0 and GPU==0 and Fences completed.
 * @param lifetime_id Target Lifetime ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_gpu_release(brrle_lifetime_id_t lifetime_id);

/**
 * @brief Pin a GPU resource to prevent eviction.
 * @param lifetime_id Target Lifetime ID.
 * @param pin_state Pin state enum (PINNED, SCANOUT, CURSOR, etc.).
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_pin(brrle_lifetime_id_t lifetime_id, brrle_pin_state_t pin_state);

/**
 * @brief Unpin a previously pinned GPU resource.
 * @param lifetime_id Target Lifetime ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_unpin(brrle_lifetime_id_t lifetime_id);

/**
 * @brief Attach a timeline fence to a resource.
 * @param lifetime_id Target Lifetime ID.
 * @param fence_id Unique timeline fence ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_fence_attach(brrle_lifetime_id_t lifetime_id, uint64_t fence_id);

/**
 * @brief Signal a timeline fence completion.
 * @param lifetime_id Target Lifetime ID.
 * @param fence_id Target fence ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_fence_signal(brrle_lifetime_id_t lifetime_id, uint64_t fence_id);

/**
 * @brief Transition residency state through the state machine.
 * @param lifetime_id Target Lifetime ID.
 * @param target_residency Target residency location.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_transition_residency(brrle_lifetime_id_t lifetime_id, brrle_residency_t target_residency);

/**
 * @brief Dump developer inspection log for a specific resource.
 * @param lifetime_id Target Lifetime ID.
 */
void brrle_lifetime_dump(brrle_lifetime_id_t lifetime_id);

/**
 * @brief Dump all registered lifetime descriptors.
 */
void brrle_lifetime_dump_all(void);

/**
 * @brief Retrieve current BRRLE diagnostics summary.
 * @param out_diag Pointer to receive diagnostics descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t brrle_get_diagnostics(brrle_diagnostics_t* out_diag);

/**
 * @brief Validate internal integrity of all tracked resources.
 * @return true if valid, false if corruption or zombie resource detected.
 */
bool brrle_lifetime_validate_all(void);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_LIFETIME_H_ */
