#include "bvmm_sync.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

extern bvmm_result_t bfse_registry_init(void);
extern bvmm_result_t bfse_registry_shutdown(void);
extern bvmm_result_t bfse_registry_register_fence(bfse_fence_desc_t* desc, bfse_fence_id_t* out_id);
extern bvmm_result_t bfse_registry_lookup_fence(bfse_fence_id_t id, bfse_fence_desc_t** out_desc);
extern bvmm_result_t bfse_registry_unregister_fence(bfse_fence_id_t id);
extern bvmm_result_t bfse_registry_register_timeline(bfse_timeline_t* timeline, bfse_timeline_id_t* out_id);
extern bvmm_result_t bfse_registry_lookup_timeline(bfse_timeline_id_t id, bfse_timeline_t** out_timeline);

static atoms_spinlock_t  g_bfse_engine_lock;
static bool              g_bfse_active = false;
static bfse_diagnostics_t g_bfse_diag = {0};

/* Fence Recycling Pool (Phase 7K Spec) */
#define BFSE_RECYCLING_POOL_SIZE 128
static bfse_fence_desc_t* g_recycling_pool[BFSE_RECYCLING_POOL_SIZE] = {0};
static uint32_t           g_recycling_count = 0;

bvmm_result_t bfse_init(void) {
    if (g_bfse_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_bfse_engine_lock, 0);
    memset(&g_bfse_diag, 0, sizeof(bfse_diagnostics_t));
    memset(g_recycling_pool, 0, sizeof(g_recycling_pool));
    g_recycling_count = 0;

    bvmm_result_t res = bfse_registry_init();
    if (res != BVMM_SUCCESS) return res;

    g_bfse_active = true;
    return BVMM_SUCCESS;
}

bvmm_result_t bfse_shutdown(void) {
    if (!g_bfse_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_bfse_engine_lock);
    for (uint32_t i = 0; i < g_recycling_count; i++) {
        if (g_recycling_pool[i]) {
            kfree(g_recycling_pool[i]);
            g_recycling_pool[i] = NULL;
        }
    }
    g_recycling_count = 0;
    atoms_spin_unlock(&g_bfse_engine_lock);

    bfse_registry_shutdown();
    g_bfse_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bfse_timeline_create(uint32_t owner_pid, bfse_timeline_id_t* out_timeline_id) {
    if (!out_timeline_id) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_bfse_active) return BVMM_ERR_NOT_INITIALIZED;

    bfse_timeline_t* timeline = (bfse_timeline_t*)kmalloc(sizeof(bfse_timeline_t));
    if (!timeline) return BVMM_ERR_OUT_OF_MEMORY;

    memset(timeline, 0, sizeof(bfse_timeline_t));
    timeline->canary_magic  = BFSE_TIMELINE_CANARY_MAGIC;
    timeline->current_value = 0;
    timeline->owner_pid     = owner_pid;
    atoms_spinlock_init(&timeline->lock, 0);

    bfse_timeline_id_t timeline_id = BFSE_INVALID_TIMELINE_ID;
    bvmm_result_t res = bfse_registry_register_timeline(timeline, &timeline_id);
    if (res != BVMM_SUCCESS) {
        kfree(timeline);
        return res;
    }

    atoms_spin_lock(&g_bfse_engine_lock);
    g_bfse_diag.total_timelines++;
    atoms_spin_unlock(&g_bfse_engine_lock);

    *out_timeline_id = timeline_id;
    return BVMM_SUCCESS;
}

bvmm_result_t bfse_timeline_advance(bfse_timeline_id_t timeline_id, uint64_t new_value) {
    bfse_timeline_t* timeline = NULL;
    bvmm_result_t res = bfse_registry_lookup_timeline(timeline_id, &timeline);
    if (res != BVMM_SUCCESS || !timeline) return res;

    atoms_spin_lock(&timeline->lock);

    /* ABSOLUTE RULE: Monotonic timeline values must NEVER decrease */
    if (new_value <= timeline->current_value) {
        atoms_spin_unlock(&timeline->lock);
        return BVMM_ERR_INVALID_ARGUMENT;
    }

    timeline->current_value = new_value;
    atoms_spin_unlock(&timeline->lock);

    return BVMM_SUCCESS;
}

bvmm_result_t bfse_timeline_query(bfse_timeline_id_t timeline_id, uint64_t* out_value) {
    if (!out_value) return BVMM_ERR_INVALID_ARGUMENT;

    bfse_timeline_t* timeline = NULL;
    bvmm_result_t res = bfse_registry_lookup_timeline(timeline_id, &timeline);
    if (res != BVMM_SUCCESS || !timeline) return res;

    atoms_spin_lock(&timeline->lock);
    *out_value = timeline->current_value;
    atoms_spin_unlock(&timeline->lock);

    return BVMM_SUCCESS;
}

bvmm_result_t bfse_fence_create(bfse_timeline_id_t timeline_id, uint64_t target_value, bfse_queue_id_t queue_id, bfse_fence_id_t* out_fence_id) {
    if (!out_fence_id) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_bfse_active) return BVMM_ERR_NOT_INITIALIZED;

    bfse_fence_desc_t* desc = NULL;

    /* Check recycling pool first (Phase 7K) */
    atoms_spin_lock(&g_bfse_engine_lock);
    if (g_recycling_count > 0) {
        g_recycling_count--;
        desc = g_recycling_pool[g_recycling_count];
        g_recycling_pool[g_recycling_count] = NULL;
        g_bfse_diag.fence_reuse_count++;
    }
    atoms_spin_unlock(&g_bfse_engine_lock);

    if (!desc) {
        desc = (bfse_fence_desc_t*)kmalloc(sizeof(bfse_fence_desc_t));
        if (!desc) return BVMM_ERR_OUT_OF_MEMORY;
    }

    memset(desc, 0, sizeof(bfse_fence_desc_t));
    desc->canary_magic           = BFSE_FENCE_CANARY_MAGIC;
    desc->timeline_id            = timeline_id;
    desc->target_timeline_value  = target_value;
    desc->type                   = (timeline_id != BFSE_INVALID_TIMELINE_ID) ? BFSE_FENCE_TIMELINE : BFSE_FENCE_BINARY;
    desc->queue_id               = queue_id;
    desc->state                  = BFSE_STATE_UNSIGNALED;
    desc->ref_count              = 1;

    bfse_fence_id_t fence_id = BFSE_INVALID_FENCE_ID;
    bvmm_result_t res = bfse_registry_register_fence(desc, &fence_id);
    if (res != BVMM_SUCCESS) {
        kfree(desc);
        return res;
    }

    atoms_spin_lock(&g_bfse_engine_lock);
    g_bfse_diag.total_fences_created++;
    g_bfse_diag.active_fences++;
    g_bfse_diag.pending_fences++;
    atoms_spin_unlock(&g_bfse_engine_lock);

    *out_fence_id = fence_id;
    return BVMM_SUCCESS;
}

bvmm_result_t bfse_fence_destroy(bfse_fence_id_t fence_id) {
    if (fence_id == BFSE_INVALID_FENCE_ID) return BVMM_ERR_INVALID_ARGUMENT;

    bfse_fence_desc_t* desc = NULL;
    bvmm_result_t res = bfse_registry_lookup_fence(fence_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    bfse_registry_unregister_fence(fence_id);

    atoms_spin_lock(&g_bfse_engine_lock);
    if (g_bfse_diag.active_fences > 0) g_bfse_diag.active_fences--;
    if (desc->state == BFSE_STATE_UNSIGNALED && g_bfse_diag.pending_fences > 0) {
        g_bfse_diag.pending_fences--;
    }

    /* Add descriptor to recycling pool if space permits */
    if (g_recycling_count < BFSE_RECYCLING_POOL_SIZE) {
        g_recycling_pool[g_recycling_count++] = desc;
        atoms_spin_unlock(&g_bfse_engine_lock);
        return BVMM_SUCCESS;
    }
    atoms_spin_unlock(&g_bfse_engine_lock);

    kfree(desc);
    return BVMM_SUCCESS;
}

bvmm_result_t bfse_signal(bfse_fence_id_t fence_id) {
    bfse_fence_desc_t* desc = NULL;
    bvmm_result_t res = bfse_registry_lookup_fence(fence_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    desc->state = BFSE_STATE_SIGNALED;
    desc->signal_count++;

    /* Advance associated timeline if attached */
    if (desc->timeline_id != BFSE_INVALID_TIMELINE_ID) {
        bfse_timeline_advance(desc->timeline_id, desc->target_timeline_value);
    }

    /* Update BRRLE timeline fence status */
    brrle_fence_signal((brrle_lifetime_id_t)fence_id, fence_id);

    atoms_spin_lock(&g_bfse_engine_lock);
    g_bfse_diag.total_fence_signals++;
    if (g_bfse_diag.pending_fences > 0) g_bfse_diag.pending_fences--;
    g_bfse_diag.completed_fences++;
    atoms_spin_unlock(&g_bfse_engine_lock);

    return BVMM_SUCCESS;
}

bvmm_result_t bfse_wait(bfse_fence_id_t fence_id, uint64_t timeout_us) {
    (void)timeout_us;
    bfse_fence_desc_t* desc = NULL;
    bvmm_result_t res = bfse_registry_lookup_fence(fence_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    desc->wait_count++;

    atoms_spin_lock(&g_bfse_engine_lock);
    g_bfse_diag.total_fence_waits++;
    atoms_spin_unlock(&g_bfse_engine_lock);

    if (desc->timeline_id != BFSE_INVALID_TIMELINE_ID) {
        uint64_t current_val = 0;
        if (bfse_timeline_query(desc->timeline_id, &current_val) == BVMM_SUCCESS) {
            if (current_val >= desc->target_timeline_value) {
                desc->state = BFSE_STATE_SIGNALED;
                return BVMM_SUCCESS;
            }
        }
    }

    return (desc->state == BFSE_STATE_SIGNALED) ? BVMM_SUCCESS : BVMM_ERR_OUT_OF_MEMORY;
}

bvmm_result_t bfse_wait_any(const bfse_fence_id_t* fence_ids, uint32_t count, uint64_t timeout_us) {
    if (!fence_ids || count == 0) return BVMM_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < count; i++) {
        if (bfse_wait(fence_ids[i], timeout_us) == BVMM_SUCCESS) {
            return BVMM_SUCCESS;
        }
    }

    return BVMM_ERR_OUT_OF_MEMORY;
}

bvmm_result_t bfse_wait_all(const bfse_fence_id_t* fence_ids, uint32_t count, uint64_t timeout_us) {
    if (!fence_ids || count == 0) return BVMM_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < count; i++) {
        if (bfse_wait(fence_ids[i], timeout_us) != BVMM_SUCCESS) {
            return BVMM_ERR_OUT_OF_MEMORY;
        }
    }

    return BVMM_SUCCESS;
}

bvmm_result_t bfse_barrier_emit(bfse_barrier_type_t barrier_type, bfse_queue_id_t queue_id) {
    (void)barrier_type;
    (void)queue_id;
    atoms_spin_lock(&g_bfse_engine_lock);
    g_bfse_diag.barrier_count++;
    atoms_spin_unlock(&g_bfse_engine_lock);
    return BVMM_SUCCESS;
}

bvmm_result_t bfse_get_diagnostics(bfse_diagnostics_t* out_diag) {
    if (!out_diag) return BVMM_ERR_INVALID_ARGUMENT;
    atoms_spin_lock(&g_bfse_engine_lock);
    *out_diag = g_bfse_diag;
    atoms_spin_unlock(&g_bfse_engine_lock);
    return BVMM_SUCCESS;
}
