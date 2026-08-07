#ifndef _BOS_BVMM_TLSF_H_
#define _BOS_BVMM_TLSF_H_

#include "../include/bvmm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_tlsf.h
 * @brief Two-Level Segregated Fit (TLSF) O(1) Deterministic Allocator
 * 
 * Provides O(1) allocation and free time complexity with low internal
 * fragmentation (<3%), block splitting, and physical neighbor coalescing.
 */

#define TLSF_FL_COUNT          32
#define TLSF_SL_COUNT          16  /* 16 subdivisions per FL class (4-bit SL) */
#define TLSF_SL_SHIFT          4
#define TLSF_MIN_ALLOC_SIZE    16  /* Minimum allocation size in bytes */

typedef struct tlsf_block {
    size_t              size_and_flags; /* Payload size | Free flags */
    struct tlsf_block*  next_free;      /* Next free block in segregated list */
    struct tlsf_block*  prev_free;      /* Prev free block in segregated list */
    struct tlsf_block*  next_phys;      /* Physical right neighbor */
    struct tlsf_block*  prev_phys;      /* Physical left neighbor */
} tlsf_block_t;

typedef struct {
    uint32_t            fl_bitmap;
    uint32_t            sl_bitmap[TLSF_FL_COUNT];
    tlsf_block_t*       matrix[TLSF_FL_COUNT][TLSF_SL_COUNT];
    uint64_t            base_address;
    size_t              total_bytes;
    size_t              used_bytes;
    size_t              free_bytes;
    size_t              largest_free_block;
    uint32_t            alloc_count;
    uint32_t            free_count;
} tlsf_pool_t;

/**
 * @brief Initialize a TLSF allocation pool over a physical memory range.
 * @param base_address Base physical/virtual address of memory block.
 * @param size_bytes Total byte size of memory range.
 * @param out_pool Pointer to receive created TLSF pool descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_tlsf_init(uint64_t base_address, size_t size_bytes, tlsf_pool_t** out_pool);

/**
 * @brief Allocate a physical memory offset using O(1) TLSF bit-scan.
 * @param pool TLSF pool pointer.
 * @param size Requested size in bytes.
 * @param align Physical alignment constraint.
 * @param out_offset Pointer to receive allocated address offset.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_tlsf_allocate(tlsf_pool_t* pool, size_t size, size_t align, uint64_t* out_offset);

/**
 * @brief Free an allocated memory range and coalesce physical neighbors in O(1) time.
 * @param pool TLSF pool pointer.
 * @param offset Physical base address to free.
 * @param size Size of freed payload.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_tlsf_free(tlsf_pool_t* pool, uint64_t offset, size_t size);

/**
 * @brief Query the largest free contiguous block currently in the TLSF pool.
 * @param pool TLSF pool pointer.
 * @param out_largest Pointer to receive largest block size in bytes.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_tlsf_query_largest(const tlsf_pool_t* pool, size_t* out_largest);

/**
 * @brief Destroy a TLSF pool.
 * @param pool TLSF pool descriptor.
 */
void bvmm_tlsf_destroy(tlsf_pool_t* pool);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_TLSF_H_ */
