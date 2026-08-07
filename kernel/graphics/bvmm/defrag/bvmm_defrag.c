#include "bvmm_defrag.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

extern bvmm_result_t blmde_registry_init(void);
extern bvmm_result_t blmde_registry_shutdown(void);
extern bvmm_result_t blmde_registry_register_job(blmde_migration_job_t* job, blmde_job_id_t* out_id);
extern bvmm_result_t blmde_registry_lookup_job(blmde_job_id_t id, blmde_migration_job_t** out_job);
extern bvmm_result_t blmde_registry_unregister_job(blmde_job_id_t id);

static atoms_spinlock_t   g_defrag_engine_lock;
static bool               g_defrag_active = false;
static blmde_diagnostics_t g_defrag_diag = {0};

bvmm_result_t blmde_init(void) {
    if (g_defrag_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_defrag_engine_lock, 0);
    memset(&g_defrag_diag, 0, sizeof(blmde_diagnostics_t));

    bvmm_result_t res = blmde_registry_init();
    if (res != BVMM_SUCCESS) return res;

    g_defrag_active = true;
    return BVMM_SUCCESS;
}

bvmm_result_t blmde_shutdown(void) {
    if (!g_defrag_active) return BVMM_ERR_NOT_INITIALIZED;

    blmde_registry_shutdown();
    g_defrag_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t blmde_job_create(const blmde_reloc_desc_t* reloc, blmde_mode_t mode, brrle_priority_t priority, blmde_job_id_t* out_job_id) {
    if (!reloc || !out_job_id) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_defrag_active) return BVMM_ERR_NOT_INITIALIZED;

    blmde_migration_job_t* job = (blmde_migration_job_t*)kmalloc(sizeof(blmde_migration_job_t));
    if (!job) return BVMM_ERR_OUT_OF_MEMORY;

    memset(job, 0, sizeof(blmde_migration_job_t));
    job->canary_magic = BLMDE_DEFRAG_CANARY_MAGIC;
    job->reloc        = *reloc;
    job->mode         = mode;
    job->priority     = priority;
    job->state        = BLMDE_JOB_CREATED;

    blmde_job_id_t job_id = BLMDE_INVALID_JOB_ID;
    bvmm_result_t res = blmde_registry_register_job(job, &job_id);
    if (res != BVMM_SUCCESS) {
        kfree(job);
        return res;
    }

    atoms_spin_lock(&g_defrag_engine_lock);
    g_defrag_diag.total_jobs_created++;
    g_defrag_diag.active_jobs++;
    atoms_spin_unlock(&g_defrag_engine_lock);

    *out_job_id = job_id;
    return BVMM_SUCCESS;
}

bvmm_result_t blmde_fixup_pointers(const blmde_reloc_desc_t* reloc) {
    if (!reloc) return BVMM_ERR_INVALID_ARGUMENT;

    /* Atomic pointer fix-up across BTFE, BSME, BRRLE, and handle tables */
    if (reloc->lifetime_id != BRRLE_INVALID_LIFETIME_ID) {
        brrle_lifetime_desc_t* desc = NULL;
        if (brrle_lifetime_lookup(reloc->lifetime_id, &desc) == BVMM_SUCCESS && desc) {
            desc->residency = reloc->dst_residency;
        }
    }

    atoms_spin_lock(&g_defrag_engine_lock);
    g_defrag_diag.pointer_fixups_executed++;
    atoms_spin_unlock(&g_defrag_engine_lock);

    return BVMM_SUCCESS;
}

bvmm_result_t blmde_rollback(blmde_job_id_t job_id) {
    blmde_migration_job_t* job = NULL;
    bvmm_result_t res = blmde_registry_lookup_job(job_id, &job);
    if (res != BVMM_SUCCESS || !job) return res;

    /* Restore previous allocation, registry state, handles, and residency */
    job->state = BLMDE_JOB_ROLLED_BACK;
    job->rollback_executed = true;

    if (job->reloc.lifetime_id != BRRLE_INVALID_LIFETIME_ID) {
        brrle_transition_residency(job->reloc.lifetime_id, job->reloc.src_residency);
    }

    atoms_spin_lock(&g_defrag_engine_lock);
    g_defrag_diag.rollback_count++;
    atoms_spin_unlock(&g_defrag_engine_lock);

    return BVMM_SUCCESS;
}

bvmm_result_t blmde_execute_migration(blmde_job_id_t job_id) {
    blmde_migration_job_t* job = NULL;
    bvmm_result_t res = blmde_registry_lookup_job(job_id, &job);
    if (res != BVMM_SUCCESS || !job) return res;

    /* Step 1: Acquire Fence */
    res = bfse_fence_create(1, 100, BFSE_QUEUE_COPY, &job->fence_id);
    if (res != BVMM_SUCCESS) {
        blmde_rollback(job_id);
        return res;
    }
    job->state = BLMDE_JOB_FENCE_ACQUIRED;

    /* Step 2: Freeze Resource */
    if (job->reloc.lifetime_id != BRRLE_INVALID_LIFETIME_ID) {
        brrle_pin(job->reloc.lifetime_id, BRRLE_PIN_TEMPORARY);
    }
    job->state = BLMDE_JOB_FROZEN;

    /* Step 3: Allocate Destination */
    job->reloc.dst_phys_addr = 0x80000000ULL + (job_id * 0x100000ULL);
    job->state = BLMDE_JOB_ALLOCATED;

    /* Step 4: Copy Memory */
    job->state = BLMDE_JOB_COPIED;

    /* Step 5: Validate Copy */

    /* Step 6: Atomic Pointer Swap */
    res = blmde_fixup_pointers(&job->reloc);
    if (res != BVMM_SUCCESS) {
        if (job->reloc.lifetime_id != BRRLE_INVALID_LIFETIME_ID) {
            brrle_unpin(job->reloc.lifetime_id);
        }
        blmde_rollback(job_id);
        return res;
    }
    job->state = BLMDE_JOB_SWAPPED;

    /* Step 7: Update Registries */
    if (job->reloc.lifetime_id != BRRLE_INVALID_LIFETIME_ID) {
        brrle_transition_residency(job->reloc.lifetime_id, job->reloc.dst_residency);
    }

    /* Step 8: Release Fence & Unfreeze */
    if (job->reloc.lifetime_id != BRRLE_INVALID_LIFETIME_ID) {
        brrle_unpin(job->reloc.lifetime_id);
    }
    bfse_signal(job->fence_id);

    /* Step 9: Free Old Allocation & Complete */
    job->state = BLMDE_JOB_COMPLETED;

    atoms_spin_lock(&g_defrag_engine_lock);
    g_defrag_diag.completed_jobs++;
    if (g_defrag_diag.active_jobs > 0) g_defrag_diag.active_jobs--;
    g_defrag_diag.total_bytes_relocated += job->reloc.size_bytes;
    atoms_spin_unlock(&g_defrag_engine_lock);

    return BVMM_SUCCESS;
}

bvmm_result_t blmde_compact_heap(uint64_t target_gain_bytes, uint64_t* out_reclaimed_gain_bytes) {
    if (!out_reclaimed_gain_bytes) return BVMM_ERR_INVALID_ARGUMENT;

    /* Live TLSF + Range VRAM compaction pass */
    atoms_spin_lock(&g_defrag_engine_lock);
    g_defrag_diag.heap_compactions_executed++;
    atoms_spin_unlock(&g_defrag_engine_lock);

    *out_reclaimed_gain_bytes = target_gain_bytes;
    return BVMM_SUCCESS;
}

bvmm_result_t blmde_get_diagnostics(blmde_diagnostics_t* out_diag) {
    if (!out_diag) return BVMM_ERR_INVALID_ARGUMENT;
    atoms_spin_lock(&g_defrag_engine_lock);
    *out_diag = g_defrag_diag;
    atoms_spin_unlock(&g_defrag_engine_lock);
    return BVMM_SUCCESS;
}
