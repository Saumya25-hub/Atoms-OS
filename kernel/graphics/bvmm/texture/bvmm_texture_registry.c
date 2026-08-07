#include "bvmm_texture.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

static btfe_texture_desc_t* g_texture_table[BTFE_MAX_TEXTURES] = {0};
static uint16_t             g_texture_generations[BTFE_MAX_TEXTURES] = {0};
static atoms_spinlock_t    g_texture_registry_lock;
static uint32_t             g_active_texture_count = 0;
static bool                 g_texture_registry_active = false;

#define BTFE_MAKE_ID(gen, idx)  (((uint64_t)(gen) << 32) | ((uint64_t)(idx) & 0xFFFFFFFFULL))
#define BTFE_GET_GEN(id)        ((uint16_t)((id) >> 32))
#define BTFE_GET_IDX(id)        ((uint32_t)((id) & 0xFFFFFFFFULL))

bvmm_result_t btfe_registry_init(void) {
    if (g_texture_registry_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_texture_registry_lock, 0);
    memset(g_texture_table, 0, sizeof(g_texture_table));
    memset(g_texture_generations, 0, sizeof(g_texture_generations));
    g_active_texture_count = 0;
    g_texture_registry_active = true;

    return BVMM_SUCCESS;
}

bvmm_result_t btfe_registry_shutdown(void) {
    if (!g_texture_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_texture_registry_lock);
    for (uint32_t i = 0; i < BTFE_MAX_TEXTURES; i++) {
        if (g_texture_table[i]) {
            g_texture_table[i]->canary_magic = 0;
            g_texture_table[i] = NULL;
        }
    }
    g_active_texture_count = 0;
    atoms_spin_unlock(&g_texture_registry_lock);

    g_texture_registry_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t btfe_registry_register(btfe_texture_desc_t* desc, btfe_texture_id_t* out_id) {
    if (!desc || !out_id) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_texture_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_texture_registry_lock);

    int free_slot = -1;
    for (uint32_t i = 0; i < BTFE_MAX_TEXTURES; i++) {
        if (!g_texture_table[i]) {
            free_slot = (int)i;
            break;
        }
    }

    if (free_slot < 0) {
        atoms_spin_unlock(&g_texture_registry_lock);
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    g_texture_generations[free_slot]++;
    if (g_texture_generations[free_slot] == 0) g_texture_generations[free_slot] = 1;
    uint16_t gen = g_texture_generations[free_slot];

    btfe_texture_id_t new_id = BTFE_MAKE_ID(gen, free_slot);

    desc->texture_id = new_id;
    desc->generation_id = gen;
    desc->registry_index = (uint32_t)free_slot;
    desc->canary_magic = BTFE_TEXTURE_CANARY_MAGIC;

    g_texture_table[free_slot] = desc;
    g_active_texture_count++;

    atoms_spin_unlock(&g_texture_registry_lock);

    *out_id = new_id;
    return BVMM_SUCCESS;
}

bvmm_result_t btfe_registry_lookup(btfe_texture_id_t id, btfe_texture_desc_t** out_desc) {
    if (id == BTFE_INVALID_TEXTURE_ID || !out_desc) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_texture_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    uint32_t idx = BTFE_GET_IDX(id);
    uint16_t gen = BTFE_GET_GEN(id);

    if (idx >= BTFE_MAX_TEXTURES) return BVMM_ERR_INVALID_HANDLE;

    atoms_spin_lock(&g_texture_registry_lock);

    btfe_texture_desc_t* target = g_texture_table[idx];
    if (!target || g_texture_generations[idx] != gen || target->canary_magic != BTFE_TEXTURE_CANARY_MAGIC) {
        atoms_spin_unlock(&g_texture_registry_lock);
        return BVMM_ERR_INVALID_HANDLE; /* Generation mismatch or stale handle */
    }

    atoms_spin_unlock(&g_texture_registry_lock);

    *out_desc = target;
    return BVMM_SUCCESS;
}

bvmm_result_t btfe_registry_unregister(btfe_texture_id_t id) {
    if (id == BTFE_INVALID_TEXTURE_ID) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_texture_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    uint32_t idx = BTFE_GET_IDX(id);
    uint16_t gen = BTFE_GET_GEN(id);

    if (idx >= BTFE_MAX_TEXTURES) return BVMM_ERR_INVALID_HANDLE;

    atoms_spin_lock(&g_texture_registry_lock);

    if (!g_texture_table[idx] || g_texture_generations[idx] != gen) {
        atoms_spin_unlock(&g_texture_registry_lock);
        return BVMM_ERR_INVALID_HANDLE;
    }

    g_texture_table[idx]->canary_magic = 0;
    g_texture_table[idx] = NULL;
    if (g_active_texture_count > 0) g_active_texture_count--;

    atoms_spin_unlock(&g_texture_registry_lock);
    return BVMM_SUCCESS;
}

uint32_t btfe_registry_get_active_count(void) {
    return g_active_texture_count;
}
