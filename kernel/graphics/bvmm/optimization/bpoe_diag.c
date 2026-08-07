#include "bpoe.h"
#include "kernel/drivers/display/display.h"

void bpoe_dump(void) {
    display_print("=== DPDP BPOE PRODUCTION OPTIMIZATION & FREEZE ENGINE DUMP ===\n");
    bpoe_diagnostics_t diag;
    if (bpoe_get_diagnostics(&diag) == BVMM_SUCCESS) {
        display_print("[DPDP BPOE] BVMM Version: ");
        display_print(diag.version_string);
        display_print("\n[DPDP BPOE] Architecture Frozen: ");
        display_print(diag.architecture_frozen ? "YES (LOCKED)" : "NO");
        display_print(" | Health Score: ");
        display_print_dec(diag.overall_health_score_pct);
        display_print("%\n[DPDP BPOE] Operations Executed: ");
        display_print_dec((uint32_t)diag.profile.total_operations_executed);
        display_print(" | Average FastPath Latency: ");
        display_print_dec((uint32_t)diag.profile.average_fastpath_latency_ns);
        display_print(" ns\n");
    }
    display_print("=============================================================\n");
}

bool bpoe_validate_all(void) {
    bpoe_validation_stats_t stats;
    if (bpoe_validate_full_chain(&stats) != BVMM_SUCCESS) return false;
    return (stats.architecture_defects_found == 0);
}
