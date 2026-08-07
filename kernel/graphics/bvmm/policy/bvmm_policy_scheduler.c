#include "bvmm_policy.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

static bmpre_migration_entry_t g_migration_queue[BMPRE_MAX_MIGRATIONS] = {0};
static atoms_spinlock_t        g_scheduler_lock;
static uint32_t                 g_migration_count = 0;
static bool                     g_scheduler_active = false;

bvmm_result_t bmpre_scheduler_init(void) {
    if (g_scheduler_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_scheduler_lock, 0);
    memset(g_migration_queue, 0, sizeof(g_migration_queue));
    g_migration_count = 0;
    g_scheduler_active = true;

    return BVMM_SUCCESS;
}

bvmm_result_t bmpre_scheduler_shutdown(void) {
    if (!g_scheduler_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_scheduler_lock);
    memset(g_migration_queue, 0, sizeof(g_migration_queue));
    g_migration_count = 0;
    atoms_spin_unlock(&g_scheduler_lock);

    g_scheduler_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bmpre_schedule_migration(brrle_lifetime_id_t lifetime_id, brrle_residency_t target_residency) {
    if (lifetime_id == BRRLE_INVALID_LIFETIME_ID) return BVMM_ERR_INVALID_ARGUMENT;

    atoms_spin_lock(&g_scheduler_lock);

    if (g_migration_count >= BMPRE_MAX_MIGRATIONS) {
        atoms_spin_unlock(&g_scheduler_lock);
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    uint32_t slot = g_migration_count;
    g_migration_queue[slot].lifetime_id        = lifetime_id;
    g_migration_queue[slot].current_residency  = BRRLE_RESIDENCY_VRAM;
    g_migration_queue[slot].target_residency   = target_residency;
    g_migration_queue[slot].schedule_timestamp = 1000;
    g_migration_queue[slot].is_completed       = true;

    g_migration_count++;

    /* Update BRRLE transition */
    brrle_transition_residency(lifetime_id, target_residency);

    atoms_spin_unlock(&g_scheduler_lock);
    return BVMM_SUCCESS;
}

uint32_t bmpre_scheduler_get_count(void) {
    return g_migration_count;
}
