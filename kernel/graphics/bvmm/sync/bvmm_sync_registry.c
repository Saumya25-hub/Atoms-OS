#include "bvmm_sync.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

static bfse_fence_desc_t* g_fence_table[BFSE_MAX_FENCES] = {0};
static uint16_t           g_fence_generations[BFSE_MAX_FENCES] = {0};
static bfse_timeline_t*   g_timeline_table[BFSE_MAX_TIMELINES] = {0};

static atoms_spinlock_t  g_sync_registry_lock;
static uint32_t           g_active_fence_count = 0;
static uint32_t           g_active_timeline_count = 0;
static bool               g_sync_registry_active = false;

#define BFSE_MAKE_ID(gen, idx)  (((uint64_t)(gen) << 32) | ((uint64_t)(idx) & 0xFFFFFFFFULL))
#define BFSE_GET_GEN(id)        ((uint16_t)((id) >> 32))
#define BFSE_GET_IDX(id)        ((uint32_t)((id) & 0xFFFFFFFFULL))

bvmm_result_t bfse_registry_init(void) {
    if (g_sync_registry_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_sync_registry_lock, 0);
    memset(g_fence_table, 0, sizeof(g_fence_table));
    memset(g_fence_generations, 0, sizeof(g_fence_generations));
    memset(g_timeline_table, 0, sizeof(g_timeline_table));
    g_active_fence_count = 0;
    g_active_timeline_count = 0;
    g_sync_registry_active = true;

    return BVMM_SUCCESS;
}

bvmm_result_t bfse_registry_shutdown(void) {
    if (!g_sync_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_sync_registry_lock);
    for (uint32_t i = 0; i < BFSE_MAX_FENCES; i++) {
        if (g_fence_table[i]) {
            g_fence_table[i]->canary_magic = 0;
            g_fence_table[i] = NULL;
        }
    }
    for (uint32_t i = 0; i < BFSE_MAX_TIMELINES; i++) {
        if (g_timeline_table[i]) {
            g_timeline_table[i]->canary_magic = 0;
            g_timeline_table[i] = NULL;
        }
    }
    g_active_fence_count = 0;
    g_active_timeline_count = 0;
    atoms_spin_unlock(&g_sync_registry_lock);

    g_sync_registry_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bfse_registry_register_fence(bfse_fence_desc_t* desc, bfse_fence_id_t* out_id) {
    if (!desc || !out_id) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_sync_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_sync_registry_lock);

    int free_slot = -1;
    for (uint32_t i = 0; i < BFSE_MAX_FENCES; i++) {
        if (!g_fence_table[i]) {
            free_slot = (int)i;
            break;
        }
    }

    if (free_slot < 0) {
        atoms_spin_unlock(&g_sync_registry_lock);
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    g_fence_generations[free_slot]++;
    if (g_fence_generations[free_slot] == 0) g_fence_generations[free_slot] = 1;
    uint16_t gen = g_fence_generations[free_slot];

    bfse_fence_id_t new_id = BFSE_MAKE_ID(gen, free_slot);

    desc->fence_id = new_id;
    desc->generation_id = gen;
    desc->registry_index = (uint32_t)free_slot;
    desc->canary_magic = BFSE_FENCE_CANARY_MAGIC;

    g_fence_table[free_slot] = desc;
    g_active_fence_count++;

    atoms_spin_unlock(&g_sync_registry_lock);

    *out_id = new_id;
    return BVMM_SUCCESS;
}

bvmm_result_t bfse_registry_lookup_fence(bfse_fence_id_t id, bfse_fence_desc_t** out_desc) {
    if (id == BFSE_INVALID_FENCE_ID || !out_desc) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_sync_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    uint32_t idx = BFSE_GET_IDX(id);
    uint16_t gen = BFSE_GET_GEN(id);

    if (idx >= BFSE_MAX_FENCES) return BVMM_ERR_INVALID_HANDLE;

    atoms_spin_lock(&g_sync_registry_lock);

    bfse_fence_desc_t* target = g_fence_table[idx];
    if (!target || g_fence_generations[idx] != gen || target->canary_magic != BFSE_FENCE_CANARY_MAGIC) {
        atoms_spin_unlock(&g_sync_registry_lock);
        return BVMM_ERR_INVALID_HANDLE;
    }

    atoms_spin_unlock(&g_sync_registry_lock);

    *out_desc = target;
    return BVMM_SUCCESS;
}

bvmm_result_t bfse_registry_unregister_fence(bfse_fence_id_t id) {
    if (id == BFSE_INVALID_FENCE_ID) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_sync_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    uint32_t idx = BFSE_GET_IDX(id);
    uint16_t gen = BFSE_GET_GEN(id);

    if (idx >= BFSE_MAX_FENCES) return BVMM_ERR_INVALID_HANDLE;

    atoms_spin_lock(&g_sync_registry_lock);

    if (!g_fence_table[idx] || g_fence_generations[idx] != gen) {
        atoms_spin_unlock(&g_sync_registry_lock);
        return BVMM_ERR_INVALID_HANDLE;
    }

    g_fence_table[idx]->canary_magic = 0;
    g_fence_table[idx] = NULL;
    if (g_active_fence_count > 0) g_active_fence_count--;

    atoms_spin_unlock(&g_sync_registry_lock);
    return BVMM_SUCCESS;
}

bvmm_result_t bfse_registry_register_timeline(bfse_timeline_t* timeline, bfse_timeline_id_t* out_id) {
    if (!timeline || !out_id) return BVMM_ERR_INVALID_ARGUMENT;

    atoms_spin_lock(&g_sync_registry_lock);

    int free_slot = -1;
    for (uint32_t i = 0; i < BFSE_MAX_TIMELINES; i++) {
        if (!g_timeline_table[i]) {
            free_slot = (int)i;
            break;
        }
    }

    if (free_slot < 0) {
        atoms_spin_unlock(&g_sync_registry_lock);
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    bfse_timeline_id_t new_id = (bfse_timeline_id_t)(free_slot + 1);
    timeline->timeline_id = new_id;
    timeline->canary_magic = BFSE_TIMELINE_CANARY_MAGIC;

    g_timeline_table[free_slot] = timeline;
    g_active_timeline_count++;

    atoms_spin_unlock(&g_sync_registry_lock);

    *out_id = new_id;
    return BVMM_SUCCESS;
}

bvmm_result_t bfse_registry_lookup_timeline(bfse_timeline_id_t id, bfse_timeline_t** out_timeline) {
    if (id == BFSE_INVALID_TIMELINE_ID || !out_timeline) return BVMM_ERR_INVALID_ARGUMENT;

    uint32_t idx = (uint32_t)(id - 1);
    if (idx >= BFSE_MAX_TIMELINES) return BVMM_ERR_INVALID_HANDLE;

    atoms_spin_lock(&g_sync_registry_lock);

    bfse_timeline_t* target = g_timeline_table[idx];
    if (!target || target->canary_magic != BFSE_TIMELINE_CANARY_MAGIC) {
        atoms_spin_unlock(&g_sync_registry_lock);
        return BVMM_ERR_INVALID_HANDLE;
    }

    atoms_spin_unlock(&g_sync_registry_lock);

    *out_timeline = target;
    return BVMM_SUCCESS;
}
