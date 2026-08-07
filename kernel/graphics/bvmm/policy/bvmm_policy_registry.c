#include "bvmm_policy.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

static bmpre_working_set_t g_working_sets[BMPRE_MAX_WORKING_SETS] = {0};
static atoms_spinlock_t    g_policy_registry_lock;
static uint32_t             g_active_working_sets = 0;
static bool                 g_policy_registry_active = false;

bvmm_result_t bmpre_registry_init(void) {
    if (g_policy_registry_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_policy_registry_lock, 0);
    memset(g_working_sets, 0, sizeof(g_working_sets));
    g_active_working_sets = 0;
    g_policy_registry_active = true;

    return BVMM_SUCCESS;
}

bvmm_result_t bmpre_registry_shutdown(void) {
    if (!g_policy_registry_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_policy_registry_lock);
    memset(g_working_sets, 0, sizeof(g_working_sets));
    g_active_working_sets = 0;
    atoms_spin_unlock(&g_policy_registry_lock);

    g_policy_registry_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bmpre_update_working_set(uint32_t owner_pid, int64_t delta_bytes) {
    if (owner_pid == 0) return BVMM_ERR_INVALID_ARGUMENT;

    atoms_spin_lock(&g_policy_registry_lock);

    int target_idx = -1;
    for (uint32_t i = 0; i < BMPRE_MAX_WORKING_SETS; i++) {
        if (g_working_sets[i].owner_pid == owner_pid) {
            target_idx = (int)i;
            break;
        }
    }

    if (target_idx < 0) {
        for (uint32_t i = 0; i < BMPRE_MAX_WORKING_SETS; i++) {
            if (g_working_sets[i].owner_pid == 0) {
                target_idx = (int)i;
                g_working_sets[i].owner_pid = owner_pid;
                g_active_working_sets++;
                break;
            }
        }
    }

    if (target_idx >= 0) {
        if (delta_bytes > 0) {
            g_working_sets[target_idx].resident_set_bytes += (uint64_t)delta_bytes;
            if (g_working_sets[target_idx].resident_set_bytes > g_working_sets[target_idx].peak_working_set_bytes) {
                g_working_sets[target_idx].peak_working_set_bytes = g_working_sets[target_idx].resident_set_bytes;
            }
        } else if (delta_bytes < 0) {
            uint64_t abs_delta = (uint64_t)(-delta_bytes);
            if (g_working_sets[target_idx].resident_set_bytes >= abs_delta) {
                g_working_sets[target_idx].resident_set_bytes -= abs_delta;
            } else {
                g_working_sets[target_idx].resident_set_bytes = 0;
            }
        }
    }

    atoms_spin_unlock(&g_policy_registry_lock);
    return BVMM_SUCCESS;
}

uint32_t bmpre_registry_get_active_working_sets(void) {
    return g_active_working_sets;
}
