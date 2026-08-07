#include "../include/bvmm.h"
#include "../policy/bvmm_policy.h"
#include "kernel/drivers/display/display.h"

/**
 * @file bvmm_phase8_tests.c
 * @brief Production Certification Tests for BVMM Phase 8 (Production Memory Policy, Residency & Eviction Engine BMPRE V1.0)
 */

bool bvmm_run_phase8_tests(void) {
    display_print("[BVMM TEST] Starting Phase 8 Production Memory Policy, Residency & Eviction Engine (BMPRE) Certification Suite...\n");

    if (!bvmm_is_initialized()) {
        bvmm_init();
    }

    /* 1. VRAM Budget & Pressure Calculation Test */
    bmpre_budget_t budget;
    bvmm_result_t res = bmpre_get_budget(&budget);
    if (res != BVMM_SUCCESS || budget.total_vram_bytes == 0) {
        display_print("[BVMM TEST] FAIL: VRAM budget query failed\n");
        return false;
    }

    bmpre_pressure_t pressure;
    res = bmpre_calculate_pressure(&pressure);
    if (res != BVMM_SUCCESS || pressure != BMPRE_PRESSURE_NORMAL) {
        display_print("[BVMM TEST] FAIL: Initial pressure level calculation failed\n");
        return false;
    }

    /* 2. Process Working Set Tracking Test */
    res = bmpre_update_working_set(100, 4 * 1024 * 1024ULL);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Working set footprint update failed\n");
        return false;
    }

    /* 3. Deterministic Policy Action Evaluation Test */
    bmpre_action_t action;
    res = bmpre_evaluate_policy(2 * 1024 * 1024ULL, BRRLE_PRIORITY_HIGH, 100, &action);
    if (res != BVMM_SUCCESS || action != BMPRE_ACTION_KEEP_RESIDENT) {
        display_print("[BVMM TEST] FAIL: Small allocation policy evaluation failed\n");
        return false;
    }

    /* 4. Eviction Candidate Finding & Pinned Immunity Test */
    brrle_lifetime_create_info_t c_info = {
        .resource_type     = BRRLE_RES_SURFACE,
        .initial_residency = BRRLE_RESIDENCY_VRAM,
        .priority          = BRRLE_PRIORITY_STREAMING,
        .owner_pid         = 100
    };
    brrle_lifetime_id_t pinned_id = BRRLE_INVALID_LIFETIME_ID;
    brrle_lifetime_create(&c_info, &pinned_id);
    brrle_pin(pinned_id, BRRLE_PIN_SCANOUT);

    bmpre_eviction_candidate_t candidates[8];
    uint32_t count = 0;
    res = bmpre_find_eviction_candidates(1024 * 1024, candidates, 8, &count);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Eviction candidate lookup failed\n");
        return false;
    }

    /* Verify pinned_id is NOT in candidates list */
    for (uint32_t i = 0; i < count; i++) {
        if (candidates[i].lifetime_id == pinned_id) {
            display_print("[BVMM TEST] FAIL: Pinned scanout surface was included in eviction candidates\n");
            return false;
        }
    }

    brrle_unpin(pinned_id);
    brrle_cpu_release(pinned_id);

    /* 5. Migration Scheduling Test */
    brrle_lifetime_id_t mig_id = BRRLE_INVALID_LIFETIME_ID;
    c_info.priority = BRRLE_PRIORITY_LOW;
    brrle_lifetime_create(&c_info, &mig_id);

    res = bmpre_schedule_migration(mig_id, BRRLE_RESIDENCY_GTT);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Migration scheduling failed\n");
        return false;
    }

    /* 6. Memory Advisor Recommendation Test */
    bmpre_advise_t advise;
    res = bmpre_advise(mig_id, &advise);
    if (res != BVMM_SUCCESS || (advise != BMPRE_ADVISE_MIGRATE && advise != BMPRE_ADVISE_EVICT && advise != BMPRE_ADVISE_KEEP)) {
        display_print("[BVMM TEST] FAIL: Memory Advisor recommendation failed\n");
        return false;
    }

    brrle_cpu_release(mig_id);

    /* 7. 100,000 Policy Decisions Stress Test */
    display_print("[BVMM TEST] Executing 100,000 Policy Decisions Stress Test...\n");
    for (int i = 0; i < 100000; i++) {
        bmpre_action_t stress_action;
        res = bmpre_evaluate_policy((uint64_t)(1024 + (i % 8192)), (brrle_priority_t)(i % 6), (uint32_t)(1 + (i % 16)), &stress_action);
        if (res != BVMM_SUCCESS) {
            display_print("[BVMM TEST] FAIL: 100,000 policy decisions stress test failed at iteration ");
            display_print_dec((uint32_t)i);
            display_print("\n");
            return false;
        }
    }
    display_print("[BVMM TEST] 100,000 Policy Decisions Stress Test: PASS\n");

    /* 8. Diagnostics & Inspector Dump */
    bmpre_policy_dump();

    display_print("[BVMM TEST] PASS: All Phase 8 Production Memory Policy, Residency & Eviction Engine Certification Tests Passed!\n");
    return true;
}
