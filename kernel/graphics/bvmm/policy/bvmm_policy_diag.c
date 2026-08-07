#include "bvmm_policy.h"
#include "kernel/drivers/display/display.h"

void bmpre_policy_dump(void) {
    display_print("=== DPDP BMPRE MEMORY POLICY ENGINE DIAGNOSTICS DUMP ===\n");
    bmpre_diagnostics_t diag;
    if (bmpre_get_diagnostics(&diag) == BVMM_SUCCESS) {
        display_print("[DPDP BMPRE] Pressure Level: ");
        display_print_dec((uint32_t)diag.current_pressure);
        display_print(" | Policy Decisions: ");
        display_print_dec((uint32_t)diag.total_policy_decisions);
        display_print(" | Evictions: ");
        display_print_dec((uint32_t)diag.total_evictions_executed);
        display_print("\n[DPDP BMPRE] Pinned Hits: ");
        display_print_dec(diag.pinned_protection_hits);
        display_print(" | Fence Hits: ");
        display_print_dec(diag.fence_protection_hits);
        display_print("\n");
    }

    bmpre_budget_t budget;
    if (bmpre_get_budget(&budget) == BVMM_SUCCESS) {
        display_print("[DPDP BMPRE] VRAM Total: ");
        display_print_dec((uint32_t)(budget.total_vram_bytes / (1024 * 1024)));
        display_print(" MB | Free: ");
        display_print_dec((uint32_t)(budget.free_vram_bytes / (1024 * 1024)));
        display_print(" MB | Resident: ");
        display_print_dec((uint32_t)(budget.resident_vram_bytes / (1024 * 1024)));
        display_print(" MB\n");
    }
    display_print("========================================================\n");
}

bool bmpre_validate_all(void) {
    bmpre_diagnostics_t diag;
    if (bmpre_get_diagnostics(&diag) != BVMM_SUCCESS) return false;
    return (diag.validation_failures == 0);
}
