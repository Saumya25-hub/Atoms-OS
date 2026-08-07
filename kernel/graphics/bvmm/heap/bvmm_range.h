#ifndef _BOS_BVMM_RANGE_H_
#define _BOS_BVMM_RANGE_H_

#include "../include/bvmm_types.h"
#include "bvmm_heap.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_range.h
 * @brief Spatial Range Allocator (drm_mm Style) Header
 * 
 * Manages contiguous physical memory ranges, base address alignment padding,
 * Best-Fit/First-Fit range searches, and neighbor range merging.
 */

typedef struct bvmm_range_node {
    uint64_t                start_address;
    size_t                  size_bytes;
    size_t                  alignment_bytes;
    bool                    allocated;
    uint32_t                canary_magic;  /* 0xDEADBEEF */
    struct bvmm_range_node* next;
    struct bvmm_range_node* prev;
} bvmm_range_node_t;

typedef struct {
    uint64_t                base_address;
    size_t                  total_bytes;
    size_t                  used_bytes;
    size_t                  free_bytes;
    size_t                  largest_free_block;
    uint32_t                node_count;
    bvmm_range_node_t*      head;
} bvmm_range_manager_t;

/**
 * @brief Initialize a Spatial Range Allocator manager.
 * @param base_address Starting physical base address.
 * @param size_bytes Capacity in bytes.
 * @param out_mgr Pointer to receive created manager.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_range_init(uint64_t base_address, size_t size_bytes, bvmm_range_manager_t** out_mgr);

/**
 * @brief Allocate a physical range satisfying alignment and search policy.
 * @param mgr Range manager pointer.
 * @param size Requested size.
 * @param align Physical alignment.
 * @param policy Search policy (Best-Fit, First-Fit, Exact-Fit, Large-Object).
 * @param out_offset Pointer to receive allocated physical base address.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_range_alloc(bvmm_range_manager_t* mgr,
                               size_t size,
                               size_t align,
                               bvmm_alloc_policy_t policy,
                               uint64_t* out_offset);

/**
 * @brief Free a physical range and coalesce adjacent free ranges.
 * @param mgr Range manager pointer.
 * @param offset Base address of range to free.
 * @param size Size of freed range.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_range_free(bvmm_range_manager_t* mgr, uint64_t offset, size_t size);

/**
 * @brief Query largest contiguous free block in the range manager.
 * @param mgr Range manager pointer.
 * @param out_largest Pointer to receive largest block size.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_range_query_largest(const bvmm_range_manager_t* mgr, size_t* out_largest);

/**
 * @brief Destroy a Spatial Range Allocator manager.
 * @param mgr Range manager pointer.
 */
void bvmm_range_destroy(bvmm_range_manager_t* mgr);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_RANGE_H_ */
