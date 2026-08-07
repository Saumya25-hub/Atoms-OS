#include "bghal.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

static bghal_gpu_device_t* g_gpu_table[BGHAL_MAX_GPUS] = {0};
static uint16_t            g_gpu_generations[BGHAL_MAX_GPUS] = {0};
static atoms_spinlock_t   g_hal_registry_lock;
static uint32_t            g_active_gpu_count = 0;
static bool                g_hal_registry_active = false;

#define BGHAL_MAKE_ID(gen, idx)  (((uint64_t)(gen) << 32) | ((uint64_t)(idx) & 0xFFFFFFFFULL))
#define BGHAL_GET_GEN(id)        ((uint16_t)((id) >> 32))
#define BGHAL_GET_IDX(id)        ((uint32_t)((id) & 0xFFFFFFFFULL))

bvmm_result_t bghal_registry_init(void) {
    if (g_hal_registry_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_hal_registry_lock, 0);
    memset(g_gpu_table, 0, sizeof(g_gpu_table));
    memset(g_gpu_generations, 0, sizeof(g_gpu_generations));
    g_active_gpu_count = 0;
    g_hal_registry_active = true;

    return BVMM_SUCCESS;
}

bvmm_result_t bghal_registry_shutdown(void) {
    if (!g_hal_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_hal_registry_lock);
    for (uint32_t i = 0; i < BGHAL_MAX_GPUS; i++) {
        if (g_gpu_table[i]) {
            g_gpu_table[i]->canary_magic = 0;
            g_gpu_table[i] = NULL;
        }
    }
    g_active_gpu_count = 0;
    atoms_spin_unlock(&g_hal_registry_lock);

    g_hal_registry_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bghal_registry_register_gpu(bghal_gpu_device_t* gpu, bghal_gpu_id_t* out_id) {
    if (!gpu || !out_id) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_hal_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_hal_registry_lock);

    int free_slot = -1;
    for (uint32_t i = 0; i < BGHAL_MAX_GPUS; i++) {
        if (!g_gpu_table[i]) {
            free_slot = (int)i;
            break;
        }
    }

    if (free_slot < 0) {
        atoms_spin_unlock(&g_hal_registry_lock);
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    g_gpu_generations[free_slot]++;
    if (g_gpu_generations[free_slot] == 0) g_gpu_generations[free_slot] = 1;
    uint16_t gen = g_gpu_generations[free_slot];

    bghal_gpu_id_t new_id = BGHAL_MAKE_ID(gen, free_slot);

    gpu->gpu_id = new_id;
    gpu->generation_id = gen;
    gpu->registry_index = (uint32_t)free_slot;
    gpu->canary_magic = BGHAL_CANARY_MAGIC;

    g_gpu_table[free_slot] = gpu;
    g_active_gpu_count++;

    atoms_spin_unlock(&g_hal_registry_lock);

    *out_id = new_id;
    return BVMM_SUCCESS;
}

bvmm_result_t bghal_registry_lookup_gpu(bghal_gpu_id_t id, bghal_gpu_device_t** out_gpu) {
    if (id == BGHAL_INVALID_GPU_ID || !out_gpu) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_hal_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    uint32_t idx = BGHAL_GET_IDX(id);
    uint16_t gen = BGHAL_GET_GEN(id);

    if (idx >= BGHAL_MAX_GPUS) return BVMM_ERR_INVALID_HANDLE;

    atoms_spin_lock(&g_hal_registry_lock);

    bghal_gpu_device_t* target = g_gpu_table[idx];
    if (!target || g_gpu_generations[idx] != gen || target->canary_magic != BGHAL_CANARY_MAGIC) {
        atoms_spin_unlock(&g_hal_registry_lock);
        return BVMM_ERR_INVALID_HANDLE;
    }

    atoms_spin_unlock(&g_hal_registry_lock);

    *out_gpu = target;
    return BVMM_SUCCESS;
}

bvmm_result_t bghal_registry_get_primary(bghal_gpu_device_t** out_gpu) {
    if (!out_gpu) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_hal_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_hal_registry_lock);

    for (uint32_t i = 0; i < BGHAL_MAX_GPUS; i++) {
        if (g_gpu_table[i] && g_gpu_table[i]->is_active) {
            *out_gpu = g_gpu_table[i];
            atoms_spin_unlock(&g_hal_registry_lock);
            return BVMM_SUCCESS;
        }
    }

    atoms_spin_unlock(&g_hal_registry_lock);
    return BVMM_ERR_INVALID_HANDLE;
}

uint32_t bghal_registry_get_active_count(void) {
    return g_active_gpu_count;
}
