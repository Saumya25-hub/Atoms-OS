#ifndef _BOS_BVMM_STATS_H_
#define _BOS_BVMM_STATS_H_

#include "../include/bvmm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_stats.h
 * @brief Real-time Telemetry, Fragmentation Analyzer & Validation Engine Header
 * 
 * Computes external/internal fragmentation indices, records real-time VRAM
 * telemetry, validates alignment/canary integrity, and provides DPDP hooks.
 */

typedef struct {
    uint32_t            external_frag_percent; /* ExtFrag = (1 - LargestFree / TotalFree) * 100 */
    uint32_t            internal_frag_percent; /* Wasted alignment padding % */
    size_t              largest_contiguous_block;
    size_t              average_hole_size;
    uint32_t            free_holes_count;
} bvmm_frag_analysis_t;

/**
 * @brief Initialize the Statistics & Diagnostics Engine.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_stats_init(void);

/**
 * @brief Record an allocation event in live telemetry counters.
 * @param domain Target domain.
 * @param pool_type Target memory pool.
 * @param size_bytes Allocated size.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_stats_record_alloc(bvmm_domain_t domain, bvmm_pool_type_t pool_type, size_t size_bytes);

/**
 * @brief Record a free event in live telemetry counters.
 * @param domain Target domain.
 * @param pool_type Target memory pool.
 * @param size_bytes Freed size.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_stats_record_free(bvmm_domain_t domain, bvmm_pool_type_t pool_type, size_t size_bytes);

/**
 * @brief Record an allocation failure event.
 * @return BVMM_SUCCESS on success.
 */
bvmm_result_t bvmm_stats_record_failure(void);

/**
 * @brief Compute real-time spatial fragmentation metrics.
 * @param out_analysis Pointer to receive fragmentation analysis descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_stats_analyze_fragmentation(bvmm_frag_analysis_t* out_analysis);

/**
 * @brief Print extended DPDP per-pool telemetry forensic dump.
 */
void bvmm_stats_dump_pool_forensics(void);

/**
 * @brief Validate allocation parameter integrity and alignment correctness.
 * @param info Allocation descriptor.
 * @return true if valid, false otherwise.
 */
bool bvmm_validate_alloc_info(const bvmm_alloc_info_t* info);

/**
 * @brief Validate allocation memory block and canary magic word.
 * @param alloc Allocation descriptor.
 * @return true if valid and uncorrupted, false otherwise.
 */
bool bvmm_validate_allocation(const bvmm_allocation_t* alloc);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_STATS_H_ */
