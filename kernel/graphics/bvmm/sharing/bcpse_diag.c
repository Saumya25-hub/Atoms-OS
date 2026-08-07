#include "bcpse.h"
#include "kernel/drivers/display/display.h"

void bcpse_dump(void) {
    display_print("=== DPDP BCPSE CROSS-PROCESS RESOURCE SHARING ENGINE DUMP ===\n");
    bcpse_diagnostics_t diag;
    if (bcpse_get_diagnostics(&diag) == BVMM_SUCCESS) {
        display_print("[DPDP BCPSE] Exported Objects: ");
        display_print_dec(diag.total_objects_exported);
        display_print(" | Imported Objects: ");
        display_print_dec(diag.total_objects_imported);
        display_print(" | Active Shared: ");
        display_print_dec(diag.active_shared_objects);
        display_print("\n[DPDP BCPSE] Zero Copy Transfers: ");
        display_print_dec((uint32_t)diag.zero_copy_transfers);
        display_print(" | Security Violations: ");
        display_print_dec(diag.security_violations);
        display_print(" | Revoked Handles: ");
        display_print_dec(diag.revoked_handles);
        display_print("\n[DPDP BCPSE] Process Cleanups: ");
        display_print_dec(diag.process_cleanups_executed);
        display_print(" | Shared VRAM: ");
        display_print_dec((uint32_t)(diag.total_shared_vram_bytes / (1024 * 1024)));
        display_print(" MB\n");
    }
    display_print("=============================================================\n");
}

bool bcpse_validate_all(void) {
    bcpse_diagnostics_t diag;
    if (bcpse_get_diagnostics(&diag) != BVMM_SUCCESS) return false;
    return (diag.security_violations == 0);
}
