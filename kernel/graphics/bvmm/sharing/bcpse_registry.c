#include "bcpse.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

static bcpse_shared_obj_t* g_shared_table[BCPSE_MAX_SHARED_OBJECTS] = {0};
static uint16_t            g_shared_generations[BCPSE_MAX_SHARED_OBJECTS] = {0};
static atoms_spinlock_t   g_sharing_registry_lock;
static uint32_t            g_active_shared_count = 0;
static bool                g_sharing_registry_active = false;

#define BCPSE_MAKE_ID(gen, idx)  (((uint64_t)(gen) << 32) | ((uint64_t)(idx) & 0xFFFFFFFFULL))
#define BCPSE_GET_GEN(id)        ((uint16_t)((id) >> 32))
#define BCPSE_GET_IDX(id)        ((uint32_t)((id) & 0xFFFFFFFFULL))

bvmm_result_t bcpse_registry_init(void) {
    if (g_sharing_registry_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_sharing_registry_lock, 0);
    memset(g_shared_table, 0, sizeof(g_shared_table));
    memset(g_shared_generations, 0, sizeof(g_shared_generations));
    g_active_shared_count = 0;
    g_sharing_registry_active = true;

    return BVMM_SUCCESS;
}

bvmm_result_t bcpse_registry_shutdown(void) {
    if (!g_sharing_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_sharing_registry_lock);
    for (uint32_t i = 0; i < BCPSE_MAX_SHARED_OBJECTS; i++) {
        if (g_shared_table[i]) {
            g_shared_table[i]->canary_magic = 0;
            g_shared_table[i] = NULL;
        }
    }
    g_active_shared_count = 0;
    atoms_spin_unlock(&g_sharing_registry_lock);

    g_sharing_registry_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bcpse_registry_register_object(bcpse_shared_obj_t* obj, bcpse_handle_t* out_handle) {
    if (!obj || !out_handle) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_sharing_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_sharing_registry_lock);

    int free_slot = -1;
    for (uint32_t i = 0; i < BCPSE_MAX_SHARED_OBJECTS; i++) {
        if (!g_shared_table[i]) {
            free_slot = (int)i;
            break;
        }
    }

    if (free_slot < 0) {
        atoms_spin_unlock(&g_sharing_registry_lock);
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    g_shared_generations[free_slot]++;
    if (g_shared_generations[free_slot] == 0) g_shared_generations[free_slot] = 1;
    uint16_t gen = g_shared_generations[free_slot];

    bcpse_handle_t new_handle = BCPSE_MAKE_ID(gen, free_slot);

    obj->shared_handle = new_handle;
    obj->generation_id = gen;
    obj->registry_index = (uint32_t)free_slot;
    obj->canary_magic = BCPSE_CANARY_MAGIC;

    g_shared_table[free_slot] = obj;
    g_active_shared_count++;

    atoms_spin_unlock(&g_sharing_registry_lock);

    *out_handle = new_handle;
    return BVMM_SUCCESS;
}

bvmm_result_t bcpse_registry_lookup_object(bcpse_handle_t handle, bcpse_shared_obj_t** out_obj) {
    if (handle == BCPSE_INVALID_HANDLE || !out_obj) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_sharing_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    uint32_t idx = BCPSE_GET_IDX(handle);
    uint16_t gen = BCPSE_GET_GEN(handle);

    if (idx >= BCPSE_MAX_SHARED_OBJECTS) return BVMM_ERR_INVALID_HANDLE;

    atoms_spin_lock(&g_sharing_registry_lock);

    bcpse_shared_obj_t* target = g_shared_table[idx];
    if (!target || g_shared_generations[idx] != gen || target->canary_magic != BCPSE_CANARY_MAGIC) {
        atoms_spin_unlock(&g_sharing_registry_lock);
        return BVMM_ERR_INVALID_HANDLE;
    }

    atoms_spin_unlock(&g_sharing_registry_lock);

    *out_obj = target;
    return BVMM_SUCCESS;
}

bvmm_result_t bcpse_registry_unregister_object(bcpse_handle_t handle) {
    if (handle == BCPSE_INVALID_HANDLE) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_sharing_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    uint32_t idx = BCPSE_GET_IDX(handle);
    uint16_t gen = BCPSE_GET_GEN(handle);

    if (idx >= BCPSE_MAX_SHARED_OBJECTS) return BVMM_ERR_INVALID_HANDLE;

    atoms_spin_lock(&g_sharing_registry_lock);

    if (!g_shared_table[idx] || g_shared_generations[idx] != gen) {
        atoms_spin_unlock(&g_sharing_registry_lock);
        return BVMM_ERR_INVALID_HANDLE;
    }

    g_shared_table[idx]->canary_magic = 0;
    g_shared_table[idx] = NULL;
    if (g_active_shared_count > 0) g_active_shared_count--;

    atoms_spin_unlock(&g_sharing_registry_lock);
    return BVMM_SUCCESS;
}

uint32_t bcpse_registry_get_active_count(void) {
    return g_active_shared_count;
}
