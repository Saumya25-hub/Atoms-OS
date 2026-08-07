#include "../include/bvmm.h"
#include "../lifetime/bvmm_lifetime.h"
#include "kernel/drivers/display/display.h"

/**
 * @file bvmm_phase6_tests.c
 * @brief Production Certification Tests for BVMM Phase 6 (Production Reference, Residency & Lifetime Engine BRRLE V1.0)
 */

bool bvmm_run_phase6_tests(void) {
    display_print("[BVMM TEST] Starting Phase 6 Reference, Residency & Lifetime Engine (BRRLE) Certification Suite...\n");

    if (!bvmm_is_initialized()) {
        bvmm_init();
    }

    /* 1. Lifetime Creation Test */
    brrle_lifetime_create_info_t create_info = {
        .resource_type     = BRRLE_RES_TEXTURE,
        .texture_id        = 1,
        .surface_id        = 1,
        .initial_residency = BRRLE_RESIDENCY_VRAM,
        .priority          = BRRLE_PRIORITY_HIGH,
        .owner_pid         = 1
    };

    brrle_lifetime_id_t life_id = BRRLE_INVALID_LIFETIME_ID;
    bvmm_result_t res = brrle_lifetime_create(&create_info, &life_id);
    if (res != BVMM_SUCCESS || life_id == BRRLE_INVALID_LIFETIME_ID) {
        display_print("[BVMM TEST] FAIL: Lifetime descriptor creation failed\n");
        return false;
    }

    /* 2. Lookup & Descriptor Verification */
    brrle_lifetime_desc_t* desc = NULL;
    res = brrle_lifetime_lookup(life_id, &desc);
    if (res != BVMM_SUCCESS || !desc || desc->cpu_refcount != 1 || desc->residency != BRRLE_RESIDENCY_VRAM) {
        display_print("[BVMM TEST] FAIL: Lifetime lookup verification failed\n");
        return false;
    }

    /* 3. Dual CPU / GPU Reference Synchronization Test */
    res = brrle_gpu_acquire(life_id);
    if (res != BVMM_SUCCESS || desc->gpu_refcount != 1) {
        display_print("[BVMM TEST] FAIL: GPU reference acquire failed\n");
        return false;
    }

    /* Premature destruction attempt should be REJECTED because GPU refcount > 0 */
    res = brrle_lifetime_destroy(life_id);
    if (res == BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Premature destruction was not rejected (GPU ref active)\n");
        return false;
    }

    /* 4. Timeline Fence Synchronization Test */
    res = brrle_fence_attach(life_id, 99001);
    if (res != BVMM_SUCCESS || desc->fence.fence_id != 99001 || desc->fence.is_signaled) {
        display_print("[BVMM TEST] FAIL: Timeline fence attach failed\n");
        return false;
    }

    /* 5. Resource Pinning & Eviction Protection Test */
    res = brrle_pin(life_id, BRRLE_PIN_SCANOUT);
    if (res != BVMM_SUCCESS || desc->pin_state != BRRLE_PIN_SCANOUT) {
        display_print("[BVMM TEST] FAIL: Resource pinning failed\n");
        return false;
    }

    /* Attempt to evict pinned scanout resource must be REJECTED */
    res = brrle_transition_residency(life_id, BRRLE_RESIDENCY_EVICTED);
    if (res == BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Illegal eviction of pinned scanout resource was not rejected\n");
        return false;
    }

    brrle_unpin(life_id);

    /* 6. Legal Residency Transition Test */
    res = brrle_transition_residency(life_id, BRRLE_RESIDENCY_GTT);
    if (res != BVMM_SUCCESS || desc->residency != BRRLE_RESIDENCY_GTT || desc->migration.migration_count != 1) {
        display_print("[BVMM TEST] FAIL: Legal residency transition failed\n");
        return false;
    }

    /* 7. Dual Reference Release & Fence Signal Cleanup */
    brrle_gpu_release(life_id); /* GPU Ref = 0 */
    brrle_fence_signal(life_id, 99001); /* Signal fence */
    brrle_cpu_release(life_id); /* CPU Ref = 0 -> Triggers auto zero-ref cleanup */

    /* Lookup should now fail due to auto-destruction */
    brrle_lifetime_desc_t* dead_desc = NULL;
    if (brrle_lifetime_lookup(life_id, &dead_desc) == BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Zero-ref auto destruction failed\n");
        return false;
    }

    /* 8. 100,000 Lifetime Transition Stress Test */
    display_print("[BVMM TEST] Executing 100,000 Lifetime Transition Stress Test...\n");
    brrle_lifetime_create_info_t stress_info = {
        .resource_type     = BRRLE_RES_SURFACE,
        .initial_residency = BRRLE_RESIDENCY_VRAM,
        .priority          = BRRLE_PRIORITY_NORMAL,
        .owner_pid         = 1
    };

    for (int i = 0; i < 100000; i++) {
        brrle_lifetime_id_t stress_id = BRRLE_INVALID_LIFETIME_ID;
        res = brrle_lifetime_create(&stress_info, &stress_id);
        if (res != BVMM_SUCCESS || stress_id == BRRLE_INVALID_LIFETIME_ID) {
            display_print("[BVMM TEST] FAIL: 100,000 lifetime stress test creation failed at iteration ");
            display_print_dec((uint32_t)i);
            display_print("\n");
            return false;
        }
        brrle_cpu_release(stress_id); /* Immediate release */
    }
    display_print("[BVMM TEST] 100,000 Lifetime Transition Stress Test: PASS\n");

    /* 9. Diagnostics & Inspector Dump */
    brrle_lifetime_dump_all();

    display_print("[BVMM TEST] PASS: All Phase 6 Reference, Residency & Lifetime Engine Certification Tests Passed!\n");
    return true;
}
