#ifndef _BOS_BVMM_HEAP_H_
#define _BOS_BVMM_HEAP_H_

#include "../include/bvmm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_heap.h
 * @brief BOS VRAM Memory Manager (BVMM) Heap Manager Declarations
 * 
 * Manages physical VRAM heaps, classified memory pools, reserved physical
 * ranges, and multi-heap dispatching.
 */

#define BVMM_MAX_HEAPS          16
#define BVMM_CANARY_MAGIC       0xDEADBEEF

/* Dedicated Heap Classifications (Phase 2D) */
typedef enum {
    BVMM_HEAP_TEXTURE          = 0,  /* Texture maps, mipchains, swizzled textures */
    BVMM_HEAP_RENDER_TARGET    = 1,  /* Offscreen render targets, depth/stencil */
    BVMM_HEAP_VERTEX_BUFFER    = 2,  /* 3D Vertex streams */
    BVMM_HEAP_INDEX_BUFFER     = 3,  /* Element index streams */
    BVMM_HEAP_UNIFORM_BUFFER   = 4,  /* Constant & uniform parameter buffers */
    BVMM_HEAP_COMMAND_BUFFER   = 5,  /* Hardware command rings and execution streams */
    BVMM_HEAP_CURSOR           = 6,  /* Hardware cursor plane overlays (4KB aligned) */
    BVMM_HEAP_UPLOAD           = 7,  /* CPU Write -> GPU Read staging */
    BVMM_HEAP_READBACK         = 8,  /* GPU Write -> CPU Read staging */
    BVMM_HEAP_TEMPORARY        = 9,  /* Frame-lifetime transient scratch buffers */
    BVMM_HEAP_CLASSIFICATION_COUNT = 10
} bvmm_heap_class_t;

typedef enum {
    BVMM_POLICY_BEST_FIT       = 0,  /* Search smallest free block satisfying size */
    BVMM_POLICY_FIRST_FIT      = 1,  /* Search first free block satisfying size */
    BVMM_POLICY_EXACT_FIT      = 2,  /* Search exact matching free block */
    BVMM_POLICY_LARGE_OBJECT   = 3,  /* Top-down strategy for large surfaces */
    BVMM_POLICY_ALIGNMENT      = 4   /* Search based on strict alignment padding */
} bvmm_alloc_policy_t;

/**
 * @brief Initialize the VRAM Heap Manager Subsystem.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_heap_subsystem_init(void);

/**
 * @brief Shutdown the VRAM Heap Manager Subsystem.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_heap_subsystem_shutdown(void);

/**
 * @brief Create a new physical VRAM or GTT heap.
 * @param heap_id Unique numerical identifier for the heap.
 * @param domain Memory domain (VRAM, GTT, SYSTEM, UPLOAD, READBACK).
 * @param base_address Base physical/virtual address of the backing memory.
 * @param size_bytes Total capacity of the heap in bytes.
 * @param out_heap Pointer to receive created heap descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_heap_create(uint32_t heap_id,
                               bvmm_domain_t domain,
                               uint64_t base_address,
                               size_t size_bytes,
                               bvmm_heap_t** out_heap);

/**
 * @brief Destroy a physical heap and release associated ranges.
 * @param heap Pointer to heap to destroy.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_heap_destroy(bvmm_heap_t* heap);

/**
 * @brief Allocate a physical range from a heap using the specified allocation policy.
 * @param heap Pointer to target heap.
 * @param size_bytes Requested allocation size in bytes.
 * @param alignment Required physical alignment.
 * @param policy Allocation search policy.
 * @param out_offset Pointer to receive allocated physical base address.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_heap_allocate_range(bvmm_heap_t* heap,
                                        size_t size_bytes,
                                        size_t alignment,
                                        bvmm_alloc_policy_t policy,
                                        uint64_t* out_offset);

/**
 * @brief Return an allocated physical range to the heap and coalesce neighbors.
 * @param heap Pointer to target heap.
 * @param offset Physical base address of range to free.
 * @param size_bytes Size of freed range in bytes.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_heap_free_range(bvmm_heap_t* heap, uint64_t offset, size_t size_bytes);

/**
 * @brief Find the default primary heap for a target domain and pool type.
 * @param domain Requested memory domain.
 * @param pool_type Requested memory pool classification.
 * @return Pointer to heap descriptor, or NULL if unavailable.
 */
bvmm_heap_t* bvmm_heap_get_primary(bvmm_domain_t domain, bvmm_pool_type_t pool_type);

/**
 * @brief Validate physical heap internal structures and range consistency.
 * @param heap Target heap to validate.
 * @return true if valid and uncorrupted, false otherwise.
 */
bool bvmm_heap_validate(const bvmm_heap_t* heap);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_HEAP_H_ */
