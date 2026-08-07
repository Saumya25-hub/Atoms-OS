#include "bvmm_lifetime.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

static brrle_lifetime_desc_t* g_lifetime_table[BRRLE_MAX_LIFETIMES] = {0};
static uint16_t               g_lifetime_generations[BRRLE_MAX_LIFETIMES] = {0};
static atoms_spinlock_t      g_lifetime_registry_lock;
static uint32_t               g_active_lifetime_count = 0;
static bool                   g_lifetime_registry_active = false;

#define BRRLE_MAKE_ID(gen, idx)  (((uint64_t)(gen) << 32) | ((uint64_t)(idx) & 0xFFFFFFFFULL))
#define BRRLE_GET_GEN(id)        ((uint16_t)((id) >> 32))
#define BRRLE_GET_IDX(id)        ((uint32_t)((id) & 0xFFFFFFFFULL))

bvmm_result_t brrle_registry_init(void) {
    if (g_lifetime_registry_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_lifetime_registry_lock, 0);
    memset(g_lifetime_table, 0, sizeof(g_lifetime_table));
    memset(g_lifetime_generations, 0, sizeof(g_lifetime_generations));
    g_active_lifetime_count = 0;
    g_lifetime_registry_active = true;

    return BVMM_SUCCESS;
}

bvmm_result_t brrle_registry_shutdown(void) {
    if (!g_lifetime_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_lifetime_registry_lock);
    for (uint32_t i = 0; i < BRRLE_MAX_LIFETIMES; i++) {
        if (g_lifetime_table[i]) {
            g_lifetime_table[i]->canary_magic = 0;
            g_lifetime_table[i] = NULL;
        }
    }
    g_active_lifetime_count = 0;
    atoms_spin_unlock(&g_lifetime_registry_lock);

    g_lifetime_registry_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t brrle_registry_register(brrle_lifetime_desc_t* desc, brrle_lifetime_id_t* out_id) {
    if (!desc || !out_id) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_lifetime_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_lifetime_registry_lock);

    int free_slot = -1;
    for (uint32_t i = 0; i < BRRLE_MAX_LIFETIMES; i++) {
        if (!g_lifetime_table[i]) {
            free_slot = (int)i;
            break;
        }
    }

    if (free_slot < 0) {
        atoms_spin_unlock(&g_lifetime_registry_lock);
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    g_lifetime_generations[free_slot]++;
    if (g_lifetime_generations[free_slot] == 0) g_lifetime_generations[free_slot] = 1;
    uint16_t gen = g_lifetime_generations[free_slot];

    brrle_lifetime_id_t new_id = BRRLE_MAKE_ID(gen, free_slot);

    desc->lifetime_id = new_id;
    desc->generation_id = gen;
    desc->registry_index = (uint32_t)free_slot;
    desc->canary_magic = BRRLE_LIFETIME_CANARY_MAGIC;

    g_lifetime_table[free_slot] = desc;
    g_active_lifetime_count++;

    atoms_spin_unlock(&g_lifetime_registry_lock);

    *out_id = new_id;
    return BVMM_SUCCESS;
}

bvmm_result_t brrle_registry_lookup(brrle_lifetime_id_t id, brrle_lifetime_desc_t** out_desc) {
    if (id == BRRLE_INVALID_LIFETIME_ID || !out_desc) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_lifetime_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    uint32_t idx = BRRLE_GET_IDX(id);
    uint16_t gen = BRRLE_GET_GEN(id);

    if (idx >= BRRLE_MAX_LIFETIMES) return BVMM_ERR_INVALID_HANDLE;

    atoms_spin_lock(&g_lifetime_registry_lock);

    brrle_lifetime_desc_t* target = g_lifetime_table[idx];
    if (!target || g_lifetime_generations[idx] != gen || target->canary_magic != BRRLE_LIFETIME_CANARY_MAGIC) {
        atoms_spin_unlock(&g_lifetime_registry_lock);
        return BVMM_ERR_INVALID_HANDLE;
    }

    atoms_spin_unlock(&g_lifetime_registry_lock);

    *out_desc = target;
    return BVMM_SUCCESS;
}

bvmm_result_t brrle_registry_unregister(brrle_lifetime_id_t id) {
    if (id == BRRLE_INVALID_LIFETIME_ID) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_lifetime_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    uint32_t idx = BRRLE_GET_IDX(id);
    uint16_t gen = BRRLE_GET_GEN(id);

    if (idx >= BRRLE_MAX_LIFETIMES) return BVMM_ERR_INVALID_HANDLE;

    atoms_spin_lock(&g_lifetime_registry_lock);

    if (!g_lifetime_table[idx] || g_lifetime_generations[idx] != gen) {
        atoms_spin_unlock(&g_lifetime_registry_lock);
        return BVMM_ERR_INVALID_HANDLE;
    }

    g_lifetime_table[idx]->canary_magic = 0;
    g_lifetime_table[idx] = NULL;
    if (g_active_lifetime_count > 0) g_active_lifetime_count--;

    atoms_spin_unlock(&g_lifetime_registry_lock);
    return BVMM_SUCCESS;
}

uint32_t brrle_registry_get_active_count(void) {
    return g_active_lifetime_count;
}
