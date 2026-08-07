#include "bvmm_sync.h"
#include "kernel/drivers/display/display.h"

extern bvmm_result_t bfse_registry_lookup_fence(bfse_fence_id_t id, bfse_fence_desc_t** out_desc);

void bfse_fence_dump(bfse_fence_id_t fence_id) {
    bfse_fence_desc_t* desc = NULL;
    if (bfse_registry_lookup_fence(fence_id, &desc) != BVMM_SUCCESS || !desc) {
        display_print("[BFSE INSPECTOR] Fence ID ");
        display_print_dec((uint32_t)fence_id);
        display_print(" NOT FOUND\n");
        return;
    }

    display_print("[BFSE INSPECTOR] Fence ID: ");
    display_print_dec((uint32_t)desc->fence_id);
    display_print(" | TimelineID: ");
    display_print_dec((uint32_t)desc->timeline_id);
    display_print(" | QueueID: ");
    display_print_dec((uint32_t)desc->queue_id);
    display_print(" | State: ");
    display_print_dec((uint32_t)desc->state);
    display_print(" | WaitCount: ");
    display_print_dec(desc->wait_count);
    display_print("\n");
}

void bfse_fence_dump_all(void) {
    display_print("=== DPDP BFSE FENCE & SYNC ENGINE DIAGNOSTICS DUMP ===\n");
    bfse_diagnostics_t diag;
    if (bfse_get_diagnostics(&diag) == BVMM_SUCCESS) {
        display_print("[DPDP BFSE] Active Fences: ");
        display_print_dec(diag.active_fences);
        display_print(" | Completed: ");
        display_print_dec(diag.completed_fences);
        display_print(" | Pending: ");
        display_print_dec(diag.pending_fences);
        display_print("\n[DPDP BFSE] Timelines: ");
        display_print_dec(diag.total_timelines);
        display_print(" | Fence Signals: ");
        display_print_dec((uint32_t)diag.total_fence_signals);
        display_print(" | Fence Waits: ");
        display_print_dec((uint32_t)diag.total_fence_waits);
        display_print("\n[DPDP BFSE] Barriers: ");
        display_print_dec(diag.barrier_count);
        display_print(" | Fence Reuse: ");
        display_print_dec(diag.fence_reuse_count);
        display_print(" | Deadlocks: ");
        display_print_dec(diag.deadlocks_detected);
        display_print("\n");
    }
    display_print("=======================================================\n");
}

bool bfse_validate_all(void) {
    bfse_diagnostics_t diag;
    if (bfse_get_diagnostics(&diag) != BVMM_SUCCESS) return false;
    return (diag.validation_failures == 0 && diag.deadlocks_detected == 0);
}
