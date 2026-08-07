#include "bvmm_lifetime.h"
#include "kernel/drivers/display/display.h"

extern bvmm_result_t brrle_registry_lookup(brrle_lifetime_id_t id, brrle_lifetime_desc_t** out_desc);
extern uint32_t brrle_registry_get_active_count(void);

void brrle_lifetime_dump(brrle_lifetime_id_t lifetime_id) {
    brrle_lifetime_desc_t* desc = NULL;
    if (brrle_registry_lookup(lifetime_id, &desc) != BVMM_SUCCESS || !desc) {
        display_print("[BRRLE INSPECTOR] Lifetime ID ");
        display_print_dec((uint32_t)lifetime_id);
        display_print(" NOT FOUND\n");
        return;
    }

    display_print("[BRRLE INSPECTOR] Lifetime ID: ");
    display_print_dec((uint32_t)desc->lifetime_id);
    display_print(" | Type: ");
    display_print_dec((uint32_t)desc->resource_type);
    display_print(" | CPU_Ref: ");
    display_print_dec(desc->cpu_refcount);
    display_print(" | GPU_Ref: ");
    display_print_dec(desc->gpu_refcount);
    display_print(" | PinState: ");
    display_print_dec((uint32_t)desc->pin_state);
    display_print(" | Residency: ");
    display_print_dec((uint32_t)desc->residency);
    display_print(" | FenceID: ");
    display_print_dec((uint32_t)desc->fence.fence_id);
    display_print(" | Signaled: ");
    display_print_dec(desc->fence.is_signaled ? 1 : 0);
    display_print("\n");
}

void brrle_lifetime_dump_all(void) {
    display_print("=== DPDP BRRLE LIFETIME ENGINE DIAGNOSTICS DUMP ===\n");
    brrle_diagnostics_t diag;
    if (brrle_get_diagnostics(&diag) == BVMM_SUCCESS) {
        display_print("[DPDP BRRLE] Tracked Resources: ");
        display_print_dec(diag.total_resources_tracked);
        display_print(" | Resident: ");
        display_print_dec(diag.resident_resources);
        display_print(" | Pinned: ");
        display_print_dec(diag.pinned_resources);
        display_print(" | Evicted: ");
        display_print_dec(diag.evicted_resources);
        display_print("\n[DPDP BRRLE] Migrations: ");
        display_print_dec(diag.migration_count_total);
        display_print(" | Fence Wait Count: ");
        display_print_dec(diag.fence_wait_count);
        display_print("\n");
    }
    display_print("===================================================\n");
}

bool brrle_lifetime_validate_all(void) {
    brrle_diagnostics_t diag;
    if (brrle_get_diagnostics(&diag) != BVMM_SUCCESS) return false;
    return (diag.validation_failures == 0 && diag.zombie_resources_detected == 0);
}
