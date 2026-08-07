#include "bvmm_lifetime.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

extern bvmm_result_t brrle_registry_init(void);
extern bvmm_result_t brrle_registry_shutdown(void);
extern bvmm_result_t brrle_registry_register(brrle_lifetime_desc_t* desc, brrle_lifetime_id_t* out_id);
extern bvmm_result_t brrle_registry_lookup(brrle_lifetime_id_t id, brrle_lifetime_desc_t** out_desc);
extern bvmm_result_t brrle_registry_unregister(brrle_lifetime_id_t id);

static atoms_spinlock_t   g_brrle_engine_lock;
static bool               g_brrle_active = false;
static brrle_diagnostics_t g_brrle_diag = {0};

bvmm_result_t brrle_init(void) {
    if (g_brrle_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_brrle_engine_lock, 0);
    memset(&g_brrle_diag, 0, sizeof(brrle_diagnostics_t));

    bvmm_result_t res = brrle_registry_init();
    if (res != BVMM_SUCCESS) return res;

    g_brrle_active = true;
    return BVMM_SUCCESS;
}

bvmm_result_t brrle_shutdown(void) {
    if (!g_brrle_active) return BVMM_ERR_NOT_INITIALIZED;

    brrle_registry_shutdown();
    g_brrle_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t brrle_lifetime_create(const brrle_lifetime_create_info_t* info, brrle_lifetime_id_t* out_lifetime_id) {
    if (!info || !out_lifetime_id) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_brrle_active) return BVMM_ERR_NOT_INITIALIZED;

    *out_lifetime_id = BRRLE_INVALID_LIFETIME_ID;

    brrle_lifetime_desc_t* desc = (brrle_lifetime_desc_t*)kmalloc(sizeof(brrle_lifetime_desc_t));
    if (!desc) return BVMM_ERR_OUT_OF_MEMORY;

    memset(desc, 0, sizeof(brrle_lifetime_desc_t));
    desc->canary_magic  = BRRLE_LIFETIME_CANARY_MAGIC;
    desc->resource_type = info->resource_type;
    desc->texture_id    = info->texture_id;
    desc->surface_id    = info->surface_id;
    desc->owner_pid     = info->owner_pid;
    desc->cpu_refcount  = 1; /* Initial CPU reference */
    desc->gpu_refcount  = 0;
    desc->state         = BRRLE_STATE_RESIDENT;
    desc->residency     = info->initial_residency;
    desc->priority      = info->priority;
    desc->pin_state     = BRRLE_PIN_UNPINNED;

    brrle_lifetime_id_t lifetime_id = BRRLE_INVALID_LIFETIME_ID;
    bvmm_result_t res = brrle_registry_register(desc, &lifetime_id);
    if (res != BVMM_SUCCESS) {
        kfree(desc);
        return res;
    }

    atoms_spin_lock(&g_brrle_engine_lock);
    g_brrle_diag.total_resources_tracked++;
    g_brrle_diag.resident_resources++;
    if ((uint32_t)info->initial_residency <= 5) {
        g_brrle_diag.residency_histogram[info->initial_residency]++;
    }
    if ((uint32_t)info->priority <= 5) {
        g_brrle_diag.priority_histogram[info->priority]++;
    }
    atoms_spin_unlock(&g_brrle_engine_lock);

    *out_lifetime_id = lifetime_id;
    return BVMM_SUCCESS;
}

bvmm_result_t brrle_lifetime_destroy(brrle_lifetime_id_t lifetime_id) {
    if (lifetime_id == BRRLE_INVALID_LIFETIME_ID) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_brrle_active) return BVMM_ERR_NOT_INITIALIZED;

    brrle_lifetime_desc_t* desc = NULL;
    bvmm_result_t res = brrle_registry_lookup(lifetime_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    /* ABSOLUTE RULE: Auto destroy ONLY when CPU_RefCount == 0 AND GPU_RefCount == 0 AND Fences completed */
    if (desc->cpu_refcount > 0 || desc->gpu_refcount > 0) {
        atoms_spin_lock(&g_brrle_engine_lock);
        g_brrle_diag.validation_failures++;
        atoms_spin_unlock(&g_brrle_engine_lock);
        return BVMM_ERR_INVALID_ARGUMENT; /* Premature destruction rejected */
    }

    if (desc->fence.fence_id != 0 && !desc->fence.is_signaled) {
        atoms_spin_lock(&g_brrle_engine_lock);
        g_brrle_diag.fence_wait_count++;
        atoms_spin_unlock(&g_brrle_engine_lock);
        return BVMM_ERR_OUT_OF_MEMORY; /* Pending GPU fence wait */
    }

    desc->state = BRRLE_STATE_DESTROYED;

    /* Cascade destroy backing Texture or Surface if owned */
    if (desc->texture_id != BTFE_INVALID_TEXTURE_ID) {
        btfe_texture_destroy(desc->texture_id);
        desc->texture_id = BTFE_INVALID_TEXTURE_ID;
    } else if (desc->surface_id != BVMM_INVALID_SURFACE_ID) {
        bvmm_surface_destroy(desc->surface_id);
        desc->surface_id = BVMM_INVALID_SURFACE_ID;
    }

    brrle_registry_unregister(lifetime_id);

    atoms_spin_lock(&g_brrle_engine_lock);
    if (g_brrle_diag.resident_resources > 0) g_brrle_diag.resident_resources--;
    atoms_spin_unlock(&g_brrle_engine_lock);

    kfree(desc);
    return BVMM_SUCCESS;
}

bvmm_result_t brrle_lifetime_lookup(brrle_lifetime_id_t lifetime_id, brrle_lifetime_desc_t** out_desc) {
    return brrle_registry_lookup(lifetime_id, out_desc);
}

bvmm_result_t brrle_cpu_acquire(brrle_lifetime_id_t lifetime_id) {
    brrle_lifetime_desc_t* desc = NULL;
    bvmm_result_t res = brrle_registry_lookup(lifetime_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    desc->cpu_refcount++;
    desc->state = BRRLE_STATE_REFERENCED;
    return BVMM_SUCCESS;
}

bvmm_result_t brrle_cpu_release(brrle_lifetime_id_t lifetime_id) {
    brrle_lifetime_desc_t* desc = NULL;
    bvmm_result_t res = brrle_registry_lookup(lifetime_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    if (desc->cpu_refcount > 0) {
        desc->cpu_refcount--;
    }

    /* Auto cleanup if dual refcounts hit zero and fences cleared */
    if (desc->cpu_refcount == 0 && desc->gpu_refcount == 0) {
        if (desc->fence.fence_id == 0 || desc->fence.is_signaled) {
            return brrle_lifetime_destroy(lifetime_id);
        }
    }

    return BVMM_SUCCESS;
}

bvmm_result_t brrle_gpu_acquire(brrle_lifetime_id_t lifetime_id) {
    brrle_lifetime_desc_t* desc = NULL;
    bvmm_result_t res = brrle_registry_lookup(lifetime_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    desc->gpu_refcount++;
    desc->state = BRRLE_STATE_REFERENCED;
    return BVMM_SUCCESS;
}

bvmm_result_t brrle_gpu_release(brrle_lifetime_id_t lifetime_id) {
    brrle_lifetime_desc_t* desc = NULL;
    bvmm_result_t res = brrle_registry_lookup(lifetime_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    if (desc->gpu_refcount > 0) {
        desc->gpu_refcount--;
    }

    if (desc->cpu_refcount == 0 && desc->gpu_refcount == 0) {
        if (desc->fence.fence_id == 0 || desc->fence.is_signaled) {
            return brrle_lifetime_destroy(lifetime_id);
        }
    }

    return BVMM_SUCCESS;
}

bvmm_result_t brrle_pin(brrle_lifetime_id_t lifetime_id, brrle_pin_state_t pin_state) {
    if (pin_state == BRRLE_PIN_UNPINNED) return BVMM_ERR_INVALID_ARGUMENT;

    brrle_lifetime_desc_t* desc = NULL;
    bvmm_result_t res = brrle_registry_lookup(lifetime_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    if (desc->pin_state == BRRLE_PIN_UNPINNED) {
        atoms_spin_lock(&g_brrle_engine_lock);
        g_brrle_diag.pinned_resources++;
        atoms_spin_unlock(&g_brrle_engine_lock);
    }

    desc->pin_state = pin_state;
    return BVMM_SUCCESS;
}

bvmm_result_t brrle_unpin(brrle_lifetime_id_t lifetime_id) {
    brrle_lifetime_desc_t* desc = NULL;
    bvmm_result_t res = brrle_registry_lookup(lifetime_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    if (desc->pin_state != BRRLE_PIN_UNPINNED) {
        desc->pin_state = BRRLE_PIN_UNPINNED;
        atoms_spin_lock(&g_brrle_engine_lock);
        if (g_brrle_diag.pinned_resources > 0) g_brrle_diag.pinned_resources--;
        atoms_spin_unlock(&g_brrle_engine_lock);
    }

    return BVMM_SUCCESS;
}

bvmm_result_t brrle_fence_attach(brrle_lifetime_id_t lifetime_id, uint64_t fence_id) {
    if (fence_id == 0) return BVMM_ERR_INVALID_ARGUMENT;

    brrle_lifetime_desc_t* desc = NULL;
    bvmm_result_t res = brrle_registry_lookup(lifetime_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    desc->fence.fence_id         = fence_id;
    desc->fence.fence_generation++;
    desc->fence.wait_count++;
    desc->fence.is_signaled      = false;

    atoms_spin_lock(&g_brrle_engine_lock);
    g_brrle_diag.fence_wait_count++;
    atoms_spin_unlock(&g_brrle_engine_lock);

    return BVMM_SUCCESS;
}

bvmm_result_t brrle_fence_signal(brrle_lifetime_id_t lifetime_id, uint64_t fence_id) {
    brrle_lifetime_desc_t* desc = NULL;
    bvmm_result_t res = brrle_registry_lookup(lifetime_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    if (desc->fence.fence_id == fence_id) {
        desc->fence.is_signaled = true;
        if (desc->fence.wait_count > 0) desc->fence.wait_count--;
    }

    /* Check if zero refcount auto cleanup can now occur */
    if (desc->cpu_refcount == 0 && desc->gpu_refcount == 0 && desc->fence.is_signaled) {
        return brrle_lifetime_destroy(lifetime_id);
    }

    return BVMM_SUCCESS;
}

bvmm_result_t brrle_transition_residency(brrle_lifetime_id_t lifetime_id, brrle_residency_t target_residency) {
    brrle_lifetime_desc_t* desc = NULL;
    bvmm_result_t res = brrle_registry_lookup(lifetime_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    /* Pinned resources cannot be evicted to GTT / System / Evicted */
    if (desc->pin_state != BRRLE_PIN_UNPINNED && target_residency == BRRLE_RESIDENCY_EVICTED) {
        atoms_spin_lock(&g_brrle_engine_lock);
        g_brrle_diag.validation_failures++;
        atoms_spin_unlock(&g_brrle_engine_lock);
        return BVMM_ERR_INVALID_ARGUMENT; /* Illegal eviction transition rejected */
    }

    desc->migration.current_residency = desc->residency;
    desc->migration.target_residency  = target_residency;
    desc->migration.migration_count++;
    desc->residency                   = target_residency;

    if (target_residency == BRRLE_RESIDENCY_EVICTED) {
        desc->state = BRRLE_STATE_EVICTED;
        atoms_spin_lock(&g_brrle_engine_lock);
        g_brrle_diag.evicted_resources++;
        atoms_spin_unlock(&g_brrle_engine_lock);
    }

    atoms_spin_lock(&g_brrle_engine_lock);
    g_brrle_diag.migration_count_total++;
    atoms_spin_unlock(&g_brrle_engine_lock);

    return BVMM_SUCCESS;
}

bvmm_result_t brrle_get_diagnostics(brrle_diagnostics_t* out_diag) {
    if (!out_diag) return BVMM_ERR_INVALID_ARGUMENT;
    atoms_spin_lock(&g_brrle_engine_lock);
    *out_diag = g_brrle_diag;
    atoms_spin_unlock(&g_brrle_engine_lock);
    return BVMM_SUCCESS;
}
