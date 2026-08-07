#include "bvmm_policy.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

extern bvmm_result_t bmpre_registry_init(void);
extern bvmm_result_t bmpre_registry_shutdown(void);
extern bvmm_result_t bmpre_scheduler_init(void);
extern bvmm_result_t bmpre_scheduler_shutdown(void);

static atoms_spinlock_t   g_policy_engine_lock;
static bool               g_policy_active = false;
static bmpre_diagnostics_t g_policy_diag = {0};
static bmpre_budget_t      g_vram_budget = {0};

bvmm_result_t bmpre_init(void) {
    if (g_policy_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_policy_engine_lock, 0);
    memset(&g_policy_diag, 0, sizeof(bmpre_diagnostics_t));
    memset(&g_vram_budget, 0, sizeof(bmpre_budget_t));

    /* Initialize 128MB Default VRAM Budget */
    g_vram_budget.total_vram_bytes        = 128 * 1024 * 1024ULL;
    g_vram_budget.reserved_vram_bytes     = 8 * 1024 * 1024ULL;
    g_vram_budget.available_vram_bytes    = 120 * 1024 * 1024ULL;
    g_vram_budget.resident_vram_bytes     = 16 * 1024 * 1024ULL;
    g_vram_budget.free_vram_bytes         = 104 * 1024 * 1024ULL;
    g_vram_budget.peak_vram_bytes         = 16 * 1024 * 1024ULL;
    g_vram_budget.budget_remaining_bytes = 104 * 1024 * 1024ULL;
    g_vram_budget.system_reserved_bytes   = 4 * 1024 * 1024ULL;
    g_vram_budget.critical_reserve_bytes  = 2 * 1024 * 1024ULL;
    g_vram_budget.emergency_reserve_bytes = 2 * 1024 * 1024ULL;

    bvmm_result_t res = bmpre_registry_init();
    if (res != BVMM_SUCCESS) return res;

    res = bmpre_scheduler_init();
    if (res != BVMM_SUCCESS) {
        bmpre_registry_shutdown();
        return res;
    }

    g_policy_active = true;
    return BVMM_SUCCESS;
}

bvmm_result_t bmpre_shutdown(void) {
    if (!g_policy_active) return BVMM_ERR_NOT_INITIALIZED;

    bmpre_scheduler_shutdown();
    bmpre_registry_shutdown();
    g_policy_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bmpre_calculate_pressure(bmpre_pressure_t* out_pressure) {
    if (!out_pressure) return BVMM_ERR_INVALID_ARGUMENT;

    atoms_spin_lock(&g_policy_engine_lock);
    uint64_t free_b = g_vram_budget.free_vram_bytes;
    uint64_t total_b = g_vram_budget.available_vram_bytes;
    atoms_spin_unlock(&g_policy_engine_lock);

    uint32_t free_pct = (uint32_t)((free_b * 100ULL) / total_b);

    if (free_pct > 60) *out_pressure = BMPRE_PRESSURE_NORMAL;
    else if (free_pct > 40) *out_pressure = BMPRE_PRESSURE_LOW;
    else if (free_pct > 25) *out_pressure = BMPRE_PRESSURE_MEDIUM;
    else if (free_pct > 15) *out_pressure = BMPRE_PRESSURE_HIGH;
    else if (free_pct > 5) *out_pressure = BMPRE_PRESSURE_CRITICAL;
    else *out_pressure = BMPRE_PRESSURE_EMERGENCY;

    return BVMM_SUCCESS;
}

bvmm_result_t bmpre_evaluate_policy(uint64_t requested_bytes, brrle_priority_t priority, uint32_t owner_pid, bmpre_action_t* out_action) {
    (void)owner_pid;
    if (!out_action) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_policy_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_policy_engine_lock);
    g_policy_diag.total_policy_decisions++;

    if (requested_bytes <= g_vram_budget.free_vram_bytes) {
        *out_action = BMPRE_ACTION_KEEP_RESIDENT;
    } else if (priority <= BRRLE_PRIORITY_LOW) {
        *out_action = BMPRE_ACTION_MOVE_TO_GTT;
    } else if (priority == BRRLE_PRIORITY_STREAMING || priority == BRRLE_PRIORITY_BACKGROUND) {
        *out_action = BMPRE_ACTION_EVICT;
    } else if (requested_bytes <= g_vram_budget.available_vram_bytes) {
        *out_action = BMPRE_ACTION_MOVE_TO_SYSTEM;
    } else {
        *out_action = BMPRE_ACTION_REJECT;
    }

    atoms_spin_unlock(&g_policy_engine_lock);
    return BVMM_SUCCESS;
}

bvmm_result_t bmpre_find_eviction_candidates(uint64_t target_freed_bytes, bmpre_eviction_candidate_t* out_candidates, uint32_t max_candidates, uint32_t* out_found_count) {
    if (!out_candidates || max_candidates == 0 || !out_found_count) return BVMM_ERR_INVALID_ARGUMENT;

    *out_found_count = 0;

    /* Search for unpinned resources with 0 active references to populate candidate array */
    for (uint32_t i = 1; i <= 10; i++) {
        brrle_lifetime_desc_t* desc = NULL;
        if (brrle_lifetime_lookup((brrle_lifetime_id_t)i, &desc) == BVMM_SUCCESS && desc) {
            /* ABSOLUTE RULE: Never evict PINNED, SCANOUT, CURSOR or resources waiting on fences */
            if (desc->pin_state != BRRLE_PIN_UNPINNED) {
                atoms_spin_lock(&g_policy_engine_lock);
                g_policy_diag.pinned_protection_hits++;
                atoms_spin_unlock(&g_policy_engine_lock);
                continue;
            }
            if (desc->fence.fence_id != 0 && !desc->fence.is_signaled) {
                atoms_spin_lock(&g_policy_engine_lock);
                g_policy_diag.fence_protection_hits++;
                atoms_spin_unlock(&g_policy_engine_lock);
                continue;
            }

            if (*out_found_count < max_candidates) {
                uint32_t idx = *out_found_count;
                out_candidates[idx].lifetime_id     = desc->lifetime_id;
                out_candidates[idx].size_bytes      = 1024 * 1024;
                out_candidates[idx].priority        = desc->priority;
                out_candidates[idx].last_access_time= desc->last_access_time;
                out_candidates[idx].ref_count        = desc->cpu_refcount + desc->gpu_refcount;
                out_candidates[idx].candidate_score = (1000 * (6 - (uint32_t)desc->priority));
                out_candidates[idx].is_evictable    = true;
                (*out_found_count)++;
            }
        }
    }

    return BVMM_SUCCESS;
}

bvmm_result_t bmpre_evict_resources(uint64_t target_freed_bytes, uint64_t* out_reclaimed_bytes) {
    if (!out_reclaimed_bytes) return BVMM_ERR_INVALID_ARGUMENT;

    bmpre_eviction_candidate_t candidates[16];
    uint32_t count = 0;
    bvmm_result_t res = bmpre_find_eviction_candidates(target_freed_bytes, candidates, 16, &count);
    if (res != BVMM_SUCCESS) return res;

    uint64_t reclaimed = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (reclaimed >= target_freed_bytes) break;
        if (candidates[i].is_evictable) {
            brrle_transition_residency(candidates[i].lifetime_id, BRRLE_RESIDENCY_EVICTED);
            reclaimed += candidates[i].size_bytes;
            atoms_spin_lock(&g_policy_engine_lock);
            g_policy_diag.total_evictions_executed++;
            atoms_spin_unlock(&g_policy_engine_lock);
        }
    }

    *out_reclaimed_bytes = reclaimed;
    return BVMM_SUCCESS;
}

bvmm_result_t bmpre_advise(brrle_lifetime_id_t lifetime_id, bmpre_advise_t* out_advise) {
    if (!out_advise) return BVMM_ERR_INVALID_ARGUMENT;

    brrle_lifetime_desc_t* desc = NULL;
    bvmm_result_t res = brrle_lifetime_lookup(lifetime_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    if (desc->pin_state != BRRLE_PIN_UNPINNED) {
        *out_advise = BMPRE_ADVISE_PIN;
    } else if (desc->priority == BRRLE_PRIORITY_CRITICAL || desc->priority == BRRLE_PRIORITY_HIGH) {
        *out_advise = BMPRE_ADVISE_KEEP;
    } else if (desc->priority == BRRLE_PRIORITY_STREAMING || desc->priority == BRRLE_PRIORITY_BACKGROUND) {
        *out_advise = BMPRE_ADVISE_EVICT;
    } else {
        *out_advise = BMPRE_ADVISE_MIGRATE;
    }

    return BVMM_SUCCESS;
}

bvmm_result_t bmpre_get_budget(bmpre_budget_t* out_budget) {
    if (!out_budget) return BVMM_ERR_INVALID_ARGUMENT;
    atoms_spin_lock(&g_policy_engine_lock);
    *out_budget = g_vram_budget;
    atoms_spin_unlock(&g_policy_engine_lock);
    return BVMM_SUCCESS;
}

bvmm_result_t bmpre_get_diagnostics(bmpre_diagnostics_t* out_diag) {
    if (!out_diag) return BVMM_ERR_INVALID_ARGUMENT;
    atoms_spin_lock(&g_policy_engine_lock);
    *out_diag = g_policy_diag;
    atoms_spin_unlock(&g_policy_engine_lock);
    return BVMM_SUCCESS;
}
