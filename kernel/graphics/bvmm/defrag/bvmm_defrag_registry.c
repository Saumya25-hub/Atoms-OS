#include "bvmm_defrag.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

static blmde_migration_job_t* g_job_table[BLMDE_MAX_MIGRATION_JOBS] = {0};
static uint16_t               g_job_generations[BLMDE_MAX_MIGRATION_JOBS] = {0};
static atoms_spinlock_t      g_defrag_registry_lock;
static uint32_t               g_active_job_count = 0;
static bool                   g_defrag_registry_active = false;

#define BLMDE_MAKE_ID(gen, idx)  (((uint64_t)(gen) << 32) | ((uint64_t)(idx) & 0xFFFFFFFFULL))
#define BLMDE_GET_GEN(id)        ((uint16_t)((id) >> 32))
#define BLMDE_GET_IDX(id)        ((uint32_t)((id) & 0xFFFFFFFFULL))

bvmm_result_t blmde_registry_init(void) {
    if (g_defrag_registry_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_defrag_registry_lock, 0);
    memset(g_job_table, 0, sizeof(g_job_table));
    memset(g_job_generations, 0, sizeof(g_job_generations));
    g_active_job_count = 0;
    g_defrag_registry_active = true;

    return BVMM_SUCCESS;
}

bvmm_result_t blmde_registry_shutdown(void) {
    if (!g_defrag_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_defrag_registry_lock);
    for (uint32_t i = 0; i < BLMDE_MAX_MIGRATION_JOBS; i++) {
        if (g_job_table[i]) {
            g_job_table[i]->canary_magic = 0;
            g_job_table[i] = NULL;
        }
    }
    g_active_job_count = 0;
    atoms_spin_unlock(&g_defrag_registry_lock);

    g_defrag_registry_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t blmde_registry_register_job(blmde_migration_job_t* job, blmde_job_id_t* out_id) {
    if (!job || !out_id) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_defrag_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_defrag_registry_lock);

    int free_slot = -1;
    for (uint32_t i = 0; i < BLMDE_MAX_MIGRATION_JOBS; i++) {
        if (!g_job_table[i]) {
            free_slot = (int)i;
            break;
        }
    }

    if (free_slot < 0) {
        atoms_spin_unlock(&g_defrag_registry_lock);
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    g_job_generations[free_slot]++;
    if (g_job_generations[free_slot] == 0) g_job_generations[free_slot] = 1;
    uint16_t gen = g_job_generations[free_slot];

    blmde_job_id_t new_id = BLMDE_MAKE_ID(gen, free_slot);

    job->job_id = new_id;
    job->generation_id = gen;
    job->registry_index = (uint32_t)free_slot;
    job->canary_magic = BLMDE_DEFRAG_CANARY_MAGIC;

    g_job_table[free_slot] = job;
    g_active_job_count++;

    atoms_spin_unlock(&g_defrag_registry_lock);

    *out_id = new_id;
    return BVMM_SUCCESS;
}

bvmm_result_t blmde_registry_lookup_job(blmde_job_id_t id, blmde_migration_job_t** out_job) {
    if (id == BLMDE_INVALID_JOB_ID || !out_job) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_defrag_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    uint32_t idx = BLMDE_GET_IDX(id);
    uint16_t gen = BLMDE_GET_GEN(id);

    if (idx >= BLMDE_MAX_MIGRATION_JOBS) return BVMM_ERR_INVALID_HANDLE;

    atoms_spin_lock(&g_defrag_registry_lock);

    blmde_migration_job_t* target = g_job_table[idx];
    if (!target || g_job_generations[idx] != gen || target->canary_magic != BLMDE_DEFRAG_CANARY_MAGIC) {
        atoms_spin_unlock(&g_defrag_registry_lock);
        return BVMM_ERR_INVALID_HANDLE;
    }

    atoms_spin_unlock(&g_defrag_registry_lock);

    *out_job = target;
    return BVMM_SUCCESS;
}

bvmm_result_t blmde_registry_unregister_job(blmde_job_id_t id) {
    if (id == BLMDE_INVALID_JOB_ID) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_defrag_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    uint32_t idx = BLMDE_GET_IDX(id);
    uint16_t gen = BLMDE_GET_GEN(id);

    if (idx >= BLMDE_MAX_MIGRATION_JOBS) return BVMM_ERR_INVALID_HANDLE;

    atoms_spin_lock(&g_defrag_registry_lock);

    if (!g_job_table[idx] || g_job_generations[idx] != gen) {
        atoms_spin_unlock(&g_defrag_registry_lock);
        return BVMM_ERR_INVALID_HANDLE;
    }

    g_job_table[idx]->canary_magic = 0;
    g_job_table[idx] = NULL;
    if (g_active_job_count > 0) g_active_job_count--;

    atoms_spin_unlock(&g_defrag_registry_lock);
    return BVMM_SUCCESS;
}
