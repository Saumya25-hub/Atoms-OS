#ifndef _BOS_BVMM_BPOE_H_
#define _BOS_BVMM_BPOE_H_

#include "../sharing/bcpse.h"
#include "kernel/core/sync/spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bpoe.h
 * @brief Production Optimization & Final Architecture Consolidation Engine (BPOE V1.0) Master Header
 * 
 * BPOE is the final production engineering layer of BVMM. It optimizes, validates,
 * profiles, and stabilizes the completed 12-phase BVMM stack, declaring a formal
 * BVMM V1.0 Production Architecture Freeze.
 */

#define BPOE_CANARY_MAGIC           0x42504F45  /* "BPOE" */

#define BVMM_VERSION_MAJOR          1
#define BVMM_VERSION_MINOR          0
#define BVMM_VERSION_PATCH          0
#define BVMM_VERSION_STRING         "1.0.0-PRODUCTION"

/* Optimization Levels */
typedef enum {
    BPOE_OPT_LEVEL_CONSERVATIVE = 0,
    BPOE_OPT_LEVEL_BALANCED     = 1,
    BPOE_OPT_LEVEL_AGGRESSIVE   = 2,
    BPOE_OPT_LEVEL_MAXIMUM      = 3
} bpoe_opt_level_t;

/* Performance Profiling Descriptor */
typedef struct {
    uint64_t total_operations_executed;
    uint64_t average_fastpath_latency_ns;
    uint64_t peak_fastpath_latency_ns;
    uint32_t descriptor_cache_hit_ratio_pct;
    uint32_t lock_contention_events;
    uint64_t total_memory_traffic_bytes;
} bpoe_profile_stats_t;

/* Architecture Validation Summary */
typedef struct {
    uint32_t total_subsystems_validated;
    uint32_t end_to_end_chain_checks_passed;
    uint32_t invalid_lifetime_violations;
    uint32_t invalid_residency_violations;
    uint32_t invalid_fence_violations;
    uint32_t architecture_defects_found;
} bpoe_validation_stats_t;

/* BPOE Diagnostics Descriptor */
typedef struct {
    bool     architecture_frozen;
    char     version_string[32];
    uint32_t overall_health_score_pct;
    bpoe_profile_stats_t    profile;
    bpoe_validation_stats_t validation;
} bpoe_diagnostics_t;

/**
 * @brief Initialize the Production Optimization Engine (BPOE V1.0).
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bpoe_init(void);

/**
 * @brief Shutdown BPOE optimization engine.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bpoe_shutdown(void);

/**
 * @brief Declare formal BVMM V1.0 Production Architecture Freeze.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bpoe_declare_architecture_freeze(void);

bvmm_result_t bpoe_profile_operation(uint64_t latency_ns, uint64_t bytes_transferred);

/**
 * @brief Run whole-stack subsystem optimization across all 11 previous BVMM phases.
 * @param level Requested optimization aggressiveness level.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bpoe_optimize_subsystems(bpoe_opt_level_t level);

/**
 * @brief Execute cross-subsystem end-to-end architecture validation chain.
 * @param out_stats Pointer to receive validation statistics.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bpoe_validate_full_chain(bpoe_validation_stats_t* out_stats);

/**
 * @brief Warm up descriptor cache and hot/cold memory layout separation.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bpoe_cache_warmup(void);

/**
 * @brief Audit and optimize lock contention across spinlocks.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bpoe_lock_audit(void);

/**
 * @brief Fast-path $O(1)$ texture lookup.
 * @param texture_id Target texture ID.
 * @param out_phys_addr Pointer to receive physical VRAM address.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bpoe_fastpath_texture_lookup(btfe_texture_id_t texture_id, uint64_t* out_phys_addr);

/**
 * @brief Fast-path $O(1)$ timeline fence lookup.
 * @param fence_id Target fence ID.
 * @param out_is_signaled Pointer to receive fence signal state.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bpoe_fastpath_fence_lookup(bfse_fence_id_t fence_id, bool* out_is_signaled);

/**
 * @brief Dump developer inspection log for BPOE operations.
 */
void bpoe_dump(void);

/**
 * @brief Retrieve current BPOE diagnostics summary.
 * @param out_diag Pointer to receive diagnostics descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bpoe_get_diagnostics(bpoe_diagnostics_t* out_diag);

/**
 * @brief Validate internal integrity of BPOE state.
 * @return true if valid, false if corruption detected.
 */
bool bpoe_validate_all(void);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_BPOE_H_ */
