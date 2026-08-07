#ifndef _BOS_BVMM_POLICY_H_
#define _BOS_BVMM_POLICY_H_

#include "../sync/bvmm_sync.h"
#include "kernel/core/sync/spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_policy.h
 * @brief Production Memory Policy, Residency & Eviction Engine (BMPRE V1.0) Master Header
 * 
 * BMPRE is the intelligent memory management authority of BOS. It evaluates VRAM budgets,
 * memory pressure, priority classes, fence readiness, working sets, and aging scores
 * to make deterministic residency, eviction, and migration decisions.
 */

#define BMPRE_POLICY_CANARY_MAGIC 0x504F4C43  /* "POLC" */
#define BMPRE_MAX_WORKING_SETS    256
#define BMPRE_MAX_MIGRATIONS      512
#define BMPRE_MAX_CANDIDATES      128

/* Policy Decision Actions (Phase 8E Spec) */
typedef enum {
    BMPRE_ACTION_KEEP_RESIDENT = 0,
    BMPRE_ACTION_MOVE_TO_GTT   = 1,
    BMPRE_ACTION_MOVE_TO_SYSTEM= 2,
    BMPRE_ACTION_EVICT         = 3,
    BMPRE_ACTION_DELAY         = 4,
    BMPRE_ACTION_REJECT        = 5
} bmpre_action_t;

/* Memory Pressure Levels (Phase 8C Spec) */
typedef enum {
    BMPRE_PRESSURE_NORMAL   = 0,
    BMPRE_PRESSURE_LOW      = 1,
    BMPRE_PRESSURE_MEDIUM   = 2,
    BMPRE_PRESSURE_HIGH     = 3,
    BMPRE_PRESSURE_CRITICAL = 4,
    BMPRE_PRESSURE_EMERGENCY= 5
} bmpre_pressure_t;

/* Memory Advisor Recommendations (Phase 8I Spec) */
typedef enum {
    BMPRE_ADVISE_KEEP    = 0,
    BMPRE_ADVISE_EVICT   = 1,
    BMPRE_ADVISE_MIGRATE = 2,
    BMPRE_ADVISE_DELAY   = 3,
    BMPRE_ADVISE_PIN     = 4,
    BMPRE_ADVISE_UNPIN   = 5
} bmpre_advise_t;

/* VRAM Memory Budget Descriptor (Phase 8B Spec) */
typedef struct {
    uint64_t total_vram_bytes;
    uint64_t reserved_vram_bytes;
    uint64_t available_vram_bytes;
    uint64_t resident_vram_bytes;
    uint64_t free_vram_bytes;
    uint64_t peak_vram_bytes;
    uint64_t budget_remaining_bytes;
    uint64_t system_reserved_bytes;
    uint64_t critical_reserve_bytes;
    uint64_t emergency_reserve_bytes;
} bmpre_budget_t;

/* Process Working Set Footprint Descriptor (Phase 8G Spec) */
typedef struct {
    uint32_t owner_pid;
    uint64_t resident_set_bytes;
    uint64_t peak_working_set_bytes;
    uint64_t inactive_set_bytes;
    uint32_t active_resource_count;
    uint32_t cold_resource_count;
    uint32_t streaming_resource_count;
} bmpre_working_set_t;

/* Eviction Candidate Descriptor (Phase 8D Spec) */
typedef struct {
    brrle_lifetime_id_t lifetime_id;
    uint64_t            size_bytes;
    brrle_priority_t    priority;
    uint64_t            last_access_time;
    uint32_t            ref_count;
    uint64_t            candidate_score;
    bool                is_evictable;
} bmpre_eviction_candidate_t;

/* Migration Schedule Entry (Phase 8F Spec) */
typedef struct {
    brrle_lifetime_id_t lifetime_id;
    brrle_residency_t   current_residency;
    brrle_residency_t   target_residency;
    uint32_t            priority;
    uint64_t            schedule_timestamp;
    uint64_t            latency_us;
    bool                is_completed;
} bmpre_migration_entry_t;

/* BMPRE Diagnostics Summary */
typedef struct {
    bmpre_pressure_t current_pressure;
    uint64_t         total_policy_decisions;
    uint64_t         total_evictions_executed;
    uint64_t         total_migrations_scheduled;
    uint32_t         active_working_sets;
    uint32_t         pinned_protection_hits;
    uint32_t         fence_protection_hits;
    uint32_t         emergency_recovery_count;
    uint32_t         validation_failures;
} bmpre_diagnostics_t;

/**
 * @brief Initialize the BOS Memory Policy, Residency & Eviction Engine (BMPRE).
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bmpre_init(void);

/**
 * @brief Shutdown BMPRE and release policy registries.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bmpre_shutdown(void);

/**
 * @brief Evaluate policy for an allocation or residency request deterministically.
 * @param requested_bytes Size of requested memory in bytes.
 * @param priority Priority class of request.
 * @param owner_pid Owner Process ID.
 * @param out_action Pointer to receive recommended policy action.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bmpre_evaluate_policy(uint64_t requested_bytes, brrle_priority_t priority, uint32_t owner_pid, bmpre_action_t* out_action);

/**
 * @brief Recalculate current VRAM memory pressure level.
 * @param out_pressure Pointer to receive pressure enum.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bmpre_calculate_pressure(bmpre_pressure_t* out_pressure);

/**
 * @brief Find and rank eviction candidates based on priority, aging, and fence status.
 * @param target_freed_bytes Memory size goal to reclaim.
 * @param out_candidates Array to receive candidate descriptors.
 * @param max_candidates Capacity of out_candidates array.
 * @param out_found_count Pointer to receive number of candidates found.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bmpre_find_eviction_candidates(uint64_t target_freed_bytes, bmpre_eviction_candidate_t* out_candidates, uint32_t max_candidates, uint32_t* out_found_count);

/**
 * @brief Execute eviction of candidates up to target_freed_bytes.
 * @param target_freed_bytes Target bytes to reclaim.
 * @param out_reclaimed_bytes Pointer to receive actual bytes reclaimed.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bmpre_evict_resources(uint64_t target_freed_bytes, uint64_t* out_reclaimed_bytes);

/**
 * @brief Schedule a migration in the migration scheduler queue.
 * @param lifetime_id Target Lifetime ID.
 * @param target_residency Target residency location.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bmpre_schedule_migration(brrle_lifetime_id_t lifetime_id, brrle_residency_t target_residency);

/**
 * @brief Retrieve Memory Advisor recommendation for a resource.
 * @param lifetime_id Target Lifetime ID.
 * @param out_advise Pointer to receive advisor recommendation.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bmpre_advise(brrle_lifetime_id_t lifetime_id, bmpre_advise_t* out_advise);

/**
 * @brief Retrieve current VRAM budget descriptor.
 * @param out_budget Pointer to receive budget descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bmpre_get_budget(bmpre_budget_t* out_budget);

/**
 * @brief Update process working set footprint.
 * @param owner_pid Process ID.
 * @param delta_bytes Positive or negative footprint change.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bmpre_update_working_set(uint32_t owner_pid, int64_t delta_bytes);

/**
 * @brief Dump developer inspection log for policy decisions.
 */
void bmpre_policy_dump(void);

/**
 * @brief Retrieve current BMPRE diagnostics summary.
 * @param out_diag Pointer to receive diagnostics descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bmpre_get_diagnostics(bmpre_diagnostics_t* out_diag);

/**
 * @brief Validate internal integrity of policy state.
 * @return true if valid, false if corruption detected.
 */
bool bmpre_validate_all(void);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_POLICY_H_ */
