#include "bvmm_surface.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

static bvmm_surface_desc_t* g_surface_table[BVMM_MAX_SURFACES] = {0};
static atoms_spinlock_t    g_surface_registry_lock;
static uint64_t             g_next_surface_id = 1;
static uint32_t             g_active_surface_count = 0;
static bool                 g_registry_initialized = false;

bvmm_result_t bvmm_surface_registry_init(void) {
    if (g_registry_initialized) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_surface_registry_lock, 0);
    memset(g_surface_table, 0, sizeof(g_surface_table));
    g_next_surface_id = 1;
    g_active_surface_count = 0;
    g_registry_initialized = true;

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_surface_registry_shutdown(void) {
    if (!g_registry_initialized) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_surface_registry_lock);
    for (uint32_t i = 0; i < BVMM_MAX_SURFACES; i++) {
        if (g_surface_table[i]) {
            g_surface_table[i]->state = BVMM_SURF_STATE_DESTROYED;
            g_surface_table[i]->canary_magic = 0;
            g_surface_table[i] = NULL;
        }
    }
    g_active_surface_count = 0;
    atoms_spin_unlock(&g_surface_registry_lock);

    g_registry_initialized = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_surface_registry_register(bvmm_surface_desc_t* desc, bvmm_surface_id_t* out_id) {
    if (!desc || !out_id) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_registry_initialized) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_surface_registry_lock);

    int free_slot = -1;
    for (uint32_t i = 0; i < BVMM_MAX_SURFACES; i++) {
        if (!g_surface_table[i]) {
            free_slot = (int)i;
            break;
        }
    }

    if (free_slot < 0) {
        atoms_spin_unlock(&g_surface_registry_lock);
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    bvmm_surface_id_t new_id = g_next_surface_id++;
    if (g_next_surface_id == 0) g_next_surface_id = 1;

    desc->surface_id = new_id;
    desc->canary_magic = BVMM_SURFACE_CANARY_MAGIC;
    desc->state = BVMM_SURF_STATE_REGISTERED;

    g_surface_table[free_slot] = desc;
    g_active_surface_count++;

    atoms_spin_unlock(&g_surface_registry_lock);

    *out_id = new_id;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_surface_registry_lookup(bvmm_surface_id_t id, bvmm_surface_desc_t** out_desc) {
    if (id == BVMM_INVALID_SURFACE_ID || !out_desc) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_registry_initialized) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_surface_registry_lock);

    bvmm_surface_desc_t* found = NULL;
    for (uint32_t i = 0; i < BVMM_MAX_SURFACES; i++) {
        if (g_surface_table[i] && g_surface_table[i]->surface_id == id) {
            if (g_surface_table[i]->canary_magic == BVMM_SURFACE_CANARY_MAGIC &&
                g_surface_table[i]->state != BVMM_SURF_STATE_DESTROYED) {
                found = g_surface_table[i];
            }
            break;
        }
    }

    atoms_spin_unlock(&g_surface_registry_lock);

    if (!found) return BVMM_ERR_INVALID_HANDLE;

    *out_desc = found;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_surface_registry_unregister(bvmm_surface_id_t id) {
    if (id == BVMM_INVALID_SURFACE_ID) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_registry_initialized) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_surface_registry_lock);

    bool removed = false;
    for (uint32_t i = 0; i < BVMM_MAX_SURFACES; i++) {
        if (g_surface_table[i] && g_surface_table[i]->surface_id == id) {
            g_surface_table[i]->state = BVMM_SURF_STATE_DESTROYED;
            g_surface_table[i]->canary_magic = 0;
            g_surface_table[i] = NULL;
            if (g_active_surface_count > 0) g_active_surface_count--;
            removed = true;
            break;
        }
    }

    atoms_spin_unlock(&g_surface_registry_lock);

    return removed ? BVMM_SUCCESS : BVMM_ERR_INVALID_HANDLE;
}

uint32_t bvmm_surface_registry_get_active_count(void) {
    return g_active_surface_count;
}
