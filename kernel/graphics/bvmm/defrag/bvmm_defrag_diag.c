#include "bvmm_defrag.h"
#include "kernel/drivers/display/display.h"

void blmde_migration_dump(void) {
    display_print("=== DPDP BLMDE LIVE MIGRATION ENGINE DIAGNOSTICS DUMP ===\n");
    blmde_diagnostics_t diag;
    if (blmde_get_diagnostics(&diag) == BVMM_SUCCESS) {
        display_print("[DPDP BLMDE] Created Jobs: ");
        display_print_dec(diag.total_jobs_created);
        display_print(" | Active: ");
        display_print_dec(diag.active_jobs);
        display_print(" | Completed: ");
        display_print_dec(diag.completed_jobs);
        display_print("\n[DPDP BLMDE] Rollbacks: ");
        display_print_dec(diag.rollback_count);
        display_print(" | Compactions: ");
        display_print_dec(diag.heap_compactions_executed);
        display_print(" | Pointer Fix-ups: ");
        display_print_dec(diag.pointer_fixups_executed);
        display_print("\n");
    }

    blmde_fragmentation_stats_t frag;
    if (blmde_analyze_fragmentation(&frag) == BVMM_SUCCESS) {
        display_print("[DPDP BLMDE] External Frag: ");
        display_print_dec(frag.external_fragmentation_pct);
        display_print("% | Health Score: ");
        display_print_dec(frag.health_score);
        display_print("%\n");
    }
    display_print("=========================================================\n");
}

bool blmde_validate_all(void) {
    blmde_diagnostics_t diag;
    if (blmde_get_diagnostics(&diag) != BVMM_SUCCESS) return false;
    return (diag.validation_failures == 0);
}
