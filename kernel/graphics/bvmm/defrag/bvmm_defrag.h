#ifndef _BOS_BVMM_DEFRAG_H_
#define _BOS_BVMM_DEFRAG_H_

#include "../policy/bvmm_policy.h"
#include "kernel/core/sync/spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_defrag.h
 * @brief Production Live Migration & Defragmentation Engine (BLMDE V1.0) Master Header
 * 
 * BLMDE is the permanent GPU Memory Movement Authority of BOS. It performs live VRAM
 * relocation, online heap compaction, zero-downtime migration, atomic pointer fix-up,
 * fragmentation recovery, and relocation rollback while applications continue running.
 */

#define BLMDE_DEFRAG_CANARY_MAGIC   0x424C4D44  /* "BLMD" */
#define BLMDE_MAX_MIGRATION_JOBS    512
#define BLMDE_INVALID_JOB_ID        0ULL

typedef uint64_t blmde_job_id_t;

/* Migration Job States (Phase 9A Spec) */
typedef enum {
    BLMDE_JOB_CREATED        = 0,
    BLMDE_JOB_FENCE_ACQUIRED = 1,
    BLMDE_JOB_FROZEN         = 2,
    BLMDE_JOB_ALLOCATED      = 3,
    BLMDE_JOB_COPIED         = 4,
    BLMDE_JOB_SWAPPED        = 5,
    BLMDE_JOB_COMPLETED      = 6,
    BLMDE_JOB_FAILED         = 7,
    BLMDE_JOB_ROLLED_BACK    = 8
} blmde_job_state_t;

/* Migration Modes (Phase 9J Spec) */
typedef enum {
    BLMDE_MODE_IMMEDIATE = 0,
    BLMDE_MODE_DEFERRED  = 1,
    BLMDE_MODE_BACKGROUND= 2,
    BLMDE_MODE_IDLE      = 3,
    BLMDE_MODE_EMERGENCY = 4
} blmde_mode_t;

/* Fragmentation Analysis Statistics (Phase 9I Spec) */
typedef struct {
    uint64_t total_heap_bytes;
    uint64_t free_heap_bytes;
    uint64_t largest_free_hole_bytes;
    uint64_t average_hole_size_bytes;
    uint32_t total_free_holes;
    uint32_t external_fragmentation_pct;
    uint32_t internal_fragmentation_pct;
    uint64_t estimated_compaction_gain_bytes;
    uint32_t health_score;
} blmde_fragmentation_stats_t;

/* Migration Relocation Descriptor (Phase 9B Spec) */
typedef struct {
    brrle_lifetime_id_t lifetime_id;
    btfe_texture_id_t   texture_id;
    bvmm_surface_id_t   surface_id;
    brrle_residency_t   src_residency;
    brrle_residency_t   dst_residency;
    uint64_t            src_phys_addr;
    uint64_t            dst_phys_addr;
    uint64_t            size_bytes;
} blmde_reloc_desc_t;

/* Primary Migration Job Descriptor Structure */
typedef struct blmde_migration_job {
    uint32_t            canary_magic;   /* 0x424C4D44 */
    blmde_job_id_t      job_id;
    uint16_t            generation_id;
    uint32_t            registry_index;
    blmde_reloc_desc_t  reloc;
    bfse_fence_id_t     fence_id;
    blmde_job_state_t   state;
    blmde_mode_t        mode;
    brrle_priority_t    priority;
    uint64_t            creation_time;
    uint64_t            completion_time;
    uint32_t            retry_count;
    bool                rollback_executed;
} blmde_migration_job_t;

/* BLMDE Diagnostics Summary */
typedef struct {
    uint32_t total_jobs_created;
    uint32_t active_jobs;
    uint32_t completed_jobs;
    uint32_t rollback_count;
    uint64_t total_bytes_relocated;
    uint32_t heap_compactions_executed;
    uint32_t pointer_fixups_executed;
    uint32_t validation_failures;
} blmde_diagnostics_t;

/**
 * @brief Initialize the BOS Live Migration & Defragmentation Engine (BLMDE).
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t blmde_init(void);

/**
 * @brief Shutdown BLMDE and release migration registries.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t blmde_shutdown(void);

/**
 * @brief Create a live migration job.
 * @param reloc Relocation descriptor specifying source/destination and IDs.
 * @param mode Scheduling mode (IMMEDIATE, BACKGROUND, etc.).
 * @param priority Priority class.
 * @param out_job_id Pointer to receive unique 64-bit Job ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t blmde_job_create(const blmde_reloc_desc_t* reloc, blmde_mode_t mode, brrle_priority_t priority, blmde_job_id_t* out_job_id);

/**
 * @brief Execute zero-downtime migration pipeline for a job.
 * @param job_id Target Migration Job ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t blmde_execute_migration(blmde_job_id_t job_id);

/**
 * @brief Execute relocation rollback if migration fails prior to atomic swap.
 * @param job_id Target Migration Job ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t blmde_rollback(blmde_job_id_t job_id);

/**
 * @brief Execute online TLSF + Range VRAM heap compaction pass.
 * @param target_gain_bytes Target bytes to recover in contiguous blocks.
 * @param out_reclaimed_gain_bytes Pointer to receive actual compaction gain.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t blmde_compact_heap(uint64_t target_gain_bytes, uint64_t* out_reclaimed_gain_bytes);

/**
 * @brief Perform atomic pointer fix-up across all subsystem registries after relocation.
 * @param reloc Pointer to relocation descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t blmde_fixup_pointers(const blmde_reloc_desc_t* reloc);

/**
 * @brief Analyze current VRAM fragmentation and compute metrics.
 * @param out_stats Pointer to receive fragmentation statistics.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t blmde_analyze_fragmentation(blmde_fragmentation_stats_t* out_stats);

/**
 * @brief Dump developer inspection log for migration operations.
 */
void blmde_migration_dump(void);

/**
 * @brief Retrieve current BLMDE diagnostics summary.
 * @param out_diag Pointer to receive diagnostics descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t blmde_get_diagnostics(blmde_diagnostics_t* out_diag);

/**
 * @brief Validate internal integrity of migration state and heap consistency.
 * @return true if valid, false if corruption detected.
 */
bool blmde_validate_all(void);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_DEFRAG_H_ */
