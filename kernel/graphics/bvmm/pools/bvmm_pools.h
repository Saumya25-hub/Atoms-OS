#ifndef _BOS_BVMM_POOLS_H_
#define _BOS_BVMM_POOLS_H_

#include "../include/bvmm_types.h"
#include "../heap/bvmm_heap.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_pools.h
 * @brief Production Memory Pool Engine (PMPE) Header
 * 
 * Manages 7 classified memory pools (Small, Medium, Large, Huge, Transient,
 * Upload, Readback) with automatic routing, lifetime enforcement, and transient resets.
 */

/* Pool Lifetimes (Phase 3 Spec) */
typedef enum {
    BVMM_LIFETIME_PERSISTENT  = 0,  /* System-wide long-lived surfaces */
    BVMM_LIFETIME_TEMPORARY   = 1,  /* Short-lived allocations */
    BVMM_LIFETIME_FRAME       = 2,  /* Frame-lifetime (cleared on present) */
    BVMM_LIFETIME_WINDOW      = 3,  /* Bound to window object lifetime */
    BVMM_LIFETIME_PROCESS     = 4,  /* Bound to process lifetime */
    BVMM_LIFETIME_DEVICE      = 5,  /* Bound to GPU device context */
    BVMM_LIFETIME_SHARED      = 6   /* Cross-process shared lifetime */
} bvmm_lifetime_t;

/* Extended Pool Flags */
#define BVMM_POOL_FLAG_PINNED         (1 << 0)  /* Non-evictable */
#define BVMM_POOL_FLAG_SCANOUT        (1 << 1)  /* Display plane scanout */
#define BVMM_POOL_FLAG_UPLOAD         (1 << 2)  /* CPU->GPU staging */
#define BVMM_POOL_FLAG_READBACK       (1 << 3)  /* GPU->CPU staging */
#define BVMM_POOL_FLAG_CPU_VISIBLE    (1 << 4)  /* Mapped via CPU BAR */
#define BVMM_POOL_FLAG_GPU_ONLY       (1 << 5)  /* VRAM internal execution */
#define BVMM_POOL_FLAG_TRANSIENT      (1 << 6)  /* Frame-lifetime ring */
#define BVMM_POOL_FLAG_SHARED         (1 << 7)  /* Cross-process IPC handle */
#define BVMM_POOL_FLAG_IMMUTABLE      (1 << 8)  /* Read-only after creation */
#define BVMM_POOL_FLAG_ALLOW_EVICTION (1 << 9)  /* Eligible for low-memory swap */

/* Per-Pool Telemetry Container */
typedef struct {
    bvmm_pool_type_t    pool_type;
    const char*         name;
    bvmm_domain_t       domain;
    size_t              min_object_size;
    size_t              max_object_size;
    size_t              alignment_policy;
    size_t              total_allocated_bytes;
    size_t              peak_bytes;
    size_t              largest_alloc_bytes;
    size_t              largest_free_block;
    uint32_t            active_objects;
    uint32_t            failed_allocations;
    uint32_t            pool_pressure_percent; /* (used / capacity) * 100 */
    uint32_t            occupancy_percent;
    uint32_t            external_frag_percent;
} bvmm_pool_telemetry_t;

typedef struct bvmm_pool_engine {
    bvmm_pool_telemetry_t stats[BVMM_POOL_COUNT];
    bvmm_heap_t*          parent_heap_vram;
    bvmm_heap_t*          parent_heap_gtt;
    bool                  active;
} bvmm_pool_engine_t;

/**
 * @brief Initialize the Production Memory Pool Engine (PMPE).
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_pools_init(void);

/**
 * @brief Shutdown the Production Memory Pool Engine.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_pools_shutdown(void);

/**
 * @brief Automatic Pool Routing Engine: Selects optimal pool class and parameters.
 * @param info Requested allocation descriptor.
 * @param out_pool_type Pointer to receive routed pool classification.
 * @param out_heap Pointer to receive target physical heap.
 * @param out_alignment Pointer to receive target alignment requirement.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_pool_route(const bvmm_alloc_info_t* info,
                              bvmm_pool_type_t* out_pool_type,
                              bvmm_heap_t** out_heap,
                              size_t* out_alignment);

/**
 * @brief Allocate memory from a specific classified pool.
 * @param pool_type Target pool classification.
 * @param info Requested allocation descriptor.
 * @param out_handle Pointer to receive 64-bit handle.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_pool_allocate(bvmm_pool_type_t pool_type,
                                 const bvmm_alloc_info_t* info,
                                 bvmm_handle_t* out_handle);

/**
 * @brief Free an allocation from its parent memory pool.
 * @param handle Handle of object to free.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_pool_free(bvmm_handle_t handle);

/**
 * @brief Perform O(1) bulk frame reset for the Transient Memory Pool.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_pool_transient_reset(void);

/**
 * @brief Retrieve detailed forensic telemetry for a specific pool class.
 * @param pool_type Target pool classification.
 * @param out_telemetry Pointer to receive telemetry container.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_pool_get_telemetry(bvmm_pool_type_t pool_type, bvmm_pool_telemetry_t* out_telemetry);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_POOLS_H_ */
