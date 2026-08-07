#ifndef _BOS_BVMM_SURFACE_H_
#define _BOS_BVMM_SURFACE_H_

#include "../include/bvmm_types.h"
#include "../pools/bvmm_pools.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_surface.h
 * @brief BOSurface Manager Engine (BSME V1.0) Header
 * 
 * Central authority for GPU surface creation, registration, reference counting,
 * locking/unlocking, state machine enforcement, and PMPE integration.
 */

#define BVMM_SURFACE_CANARY_MAGIC   0x55524643  /* "SURF" */
#define BVMM_MAX_SURFACES           4096
#define BVMM_INVALID_SURFACE_ID     0ULL

/* Opaque 64-bit Surface ID */
typedef uint64_t bvmm_surface_id_t;

/* Surface Formats (Phase 4 Spec) */
typedef enum {
    BVMM_SURF_FMT_RGBA8888  = 0,
    BVMM_SURF_FMT_RGB565    = 1,
    BVMM_SURF_FMT_ARGB8888  = 2,
    BVMM_SURF_FMT_XRGB8888  = 3,
    BVMM_SURF_FMT_R8        = 4,
    BVMM_SURF_FMT_R16       = 5,
    BVMM_SURF_FMT_R32F      = 6,
    BVMM_SURF_FMT_DEPTH24   = 7,
    BVMM_SURF_FMT_DEPTH32   = 8,
    BVMM_SURF_FMT_CUSTOM    = 9
} bvmm_surface_format_t;

/* Surface Flags (Phase 4 Spec) */
#define BVMM_SURF_FLAG_STATIC          (1 << 0)
#define BVMM_SURF_FLAG_DYNAMIC         (1 << 1)
#define BVMM_SURF_FLAG_RENDER_TARGET   (1 << 2)
#define BVMM_SURF_FLAG_DEPTH           (1 << 3)
#define BVMM_SURF_FLAG_TEXTURE         (1 << 4)
#define BVMM_SURF_FLAG_STAGING         (1 << 5)
#define BVMM_SURF_FLAG_SYSTEM_MEMORY   (1 << 6)
#define BVMM_SURF_FLAG_GPU_ONLY        (1 << 7)
#define BVMM_SURF_FLAG_CPU_VISIBLE     (1 << 8)
#define BVMM_SURF_FLAG_SHARED          (1 << 9)
#define BVMM_SURF_FLAG_PROTECTED       (1 << 10)

/* Lifetime State Machine (Phase 4 Spec) */
typedef enum {
    BVMM_SURF_STATE_CREATED    = 0,
    BVMM_SURF_STATE_ALLOCATED  = 1,
    BVMM_SURF_STATE_REGISTERED = 2,
    BVMM_SURF_STATE_READY      = 3,
    BVMM_SURF_STATE_REFERENCED = 4,
    BVMM_SURF_STATE_LOCKED     = 5,
    BVMM_SURF_STATE_UNLOCKED   = 6,
    BVMM_SURF_STATE_RELEASED   = 7,
    BVMM_SURF_STATE_DESTROYED  = 8
} bvmm_surface_state_t;

/* Surface Creation Parameters */
typedef struct {
    uint32_t                width;
    uint32_t                height;
    bvmm_surface_format_t   format;
    uint32_t                flags;
    bvmm_domain_t           preferred_domain;
    bvmm_lifetime_t         lifetime;
    uint32_t                owner_pid;
    uint32_t                owner_module;
} bvmm_surface_create_info_t;

/* Surface Lock Descriptor */
typedef struct {
    uint32_t                lock_count;
    uint32_t                lock_owner_thread_id;
    uint64_t                lock_timestamp;
    void*                   mapped_cpu_ptr;
} bvmm_surface_lock_info_t;

/* Primary Surface Descriptor */
typedef struct bvmm_surface_desc {
    uint32_t                canary_magic;    /* 0x55524643 */
    bvmm_surface_id_t       surface_id;
    uint32_t                width;
    uint32_t                height;
    uint32_t                stride_bytes;
    bvmm_surface_format_t   format;
    size_t                  size_bytes;
    uint32_t                flags;
    bvmm_pool_type_t        pool_type;
    bvmm_handle_t           alloc_handle;
    bvmm_surface_state_t    state;
    uint32_t                ref_count;
    bvmm_surface_lock_info_t lock_info;
    uint32_t                owner_pid;
    uint32_t                owner_module;
    uint64_t                creation_time;
    uint64_t                last_access_time;
    void*                   private_data;
} bvmm_surface_desc_t;

/* Surface Diagnostics Summary */
typedef struct {
    uint32_t                total_surfaces_created;
    uint32_t                alive_surfaces;
    uint32_t                destroyed_surfaces;
    uint32_t                peak_surfaces;
    size_t                  total_memory_bytes;
    size_t                  vram_memory_bytes;
    size_t                  largest_surface_bytes;
    uint32_t                allocation_failures;
    uint32_t                validation_failures;
    uint32_t                ref_leaks_detected;
    uint32_t                lock_leaks_detected;
} bvmm_surface_stats_t;

/**
 * @brief Initialize the BOSurface Manager Engine (BSME).
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_surface_engine_init(void);

/**
 * @brief Shutdown BSME and destroy all remaining surfaces.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_surface_engine_shutdown(void);

/**
 * @brief Create a new surface allocated through PMPE.
 * @param info Creation descriptor.
 * @param out_surface_id Pointer to receive unique 64-bit Surface ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_surface_create(const bvmm_surface_create_info_t* info, bvmm_surface_id_t* out_surface_id);

/**
 * @brief Destroy a surface and return its backing memory to PMPE.
 * @param surface_id Target surface ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_surface_destroy(bvmm_surface_id_t surface_id);

/**
 * @brief Lookup a surface descriptor by Surface ID.
 * @param surface_id Target surface ID.
 * @param out_desc Pointer to receive surface descriptor pointer.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_surface_lookup(bvmm_surface_id_t surface_id, bvmm_surface_desc_t** out_desc);

/**
 * @brief Clone an existing surface with a new allocation.
 * @param src_surface_id Source surface ID to clone.
 * @param out_cloned_id Pointer to receive cloned surface ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_surface_clone(bvmm_surface_id_t src_surface_id, bvmm_surface_id_t* out_cloned_id);

/**
 * @brief Resize an existing surface by re-allocating memory through PMPE.
 * @param surface_id Target surface ID.
 * @param new_width New width in pixels.
 * @param new_height New height in pixels.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_surface_resize(bvmm_surface_id_t surface_id, uint32_t new_width, uint32_t new_height);

/**
 * @brief Increment reference count of a surface.
 * @param surface_id Target surface ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_surface_ref_inc(bvmm_surface_id_t surface_id);

/**
 * @brief Decrement reference count of a surface. Auto-destroys when refcount reaches 0.
 * @param surface_id Target surface ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_surface_ref_dec(bvmm_surface_id_t surface_id);

/**
 * @brief Lock a surface for CPU access.
 * @param surface_id Target surface ID.
 * @param out_cpu_ptr Pointer to receive mapped CPU address.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_surface_lock(bvmm_surface_id_t surface_id, void** out_cpu_ptr);

/**
 * @brief Unlock a previously locked surface.
 * @param surface_id Target surface ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_surface_unlock(bvmm_surface_id_t surface_id);

/**
 * @brief Attempt non-blocking lock on a surface.
 * @param surface_id Target surface ID.
 * @param out_cpu_ptr Pointer to receive mapped CPU address.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_surface_trylock(bvmm_surface_id_t surface_id, void** out_cpu_ptr);

/**
 * @brief Dump detailed inspector log for a specific surface.
 * @param surface_id Target surface ID.
 */
void bvmm_surface_dump(bvmm_surface_id_t surface_id);

/**
 * @brief Dump all registered active surfaces in the system.
 */
void bvmm_surface_dump_all(void);

/**
 * @brief Retrieve current BSME statistics summary.
 * @param out_stats Pointer to receive statistics descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_surface_get_stats(bvmm_surface_stats_t* out_stats);

/**
 * @brief Validate internal integrity of all live registered surfaces.
 * @return true if all surfaces are valid, false if corruption detected.
 */
bool bvmm_surface_validate_all(void);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_SURFACE_H_ */
