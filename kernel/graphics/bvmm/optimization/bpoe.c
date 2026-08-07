#include "bpoe.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

static atoms_spinlock_t   g_bpoe_lock;
static bool               g_bpoe_active = false;
static bool               g_architecture_frozen = false;
static bpoe_diagnostics_t g_bpoe_diag = {0};

bvmm_result_t bpoe_init(void) {
    if (g_bpoe_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_bpoe_lock, 0);
    memset(&g_bpoe_diag, 0, sizeof(bpoe_diagnostics_t));

    g_bpoe_diag.architecture_frozen = false;
    g_bpoe_diag.overall_health_score_pct = 100;
    strncpy(g_bpoe_diag.version_string, BVMM_VERSION_STRING, sizeof(g_bpoe_diag.version_string) - 1);

    g_bpoe_active = true;
    return BVMM_SUCCESS;
}

bvmm_result_t bpoe_shutdown(void) {
    if (!g_bpoe_active) return BVMM_ERR_NOT_INITIALIZED;
    g_bpoe_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bpoe_declare_architecture_freeze(void) {
    if (!g_bpoe_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_bpoe_lock);
    g_architecture_frozen = true;
    g_bpoe_diag.architecture_frozen = true;
    atoms_spin_unlock(&g_bpoe_lock);

    return BVMM_SUCCESS;
}

bvmm_result_t bpoe_get_diagnostics(bpoe_diagnostics_t* out_diag) {
    if (!out_diag) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_bpoe_active) return BVMM_ERR_NOT_INITIALIZED;

    atoms_spin_lock(&g_bpoe_lock);
    *out_diag = g_bpoe_diag;
    atoms_spin_unlock(&g_bpoe_lock);

    return BVMM_SUCCESS;
}
