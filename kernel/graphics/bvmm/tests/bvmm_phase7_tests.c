#include "../include/bvmm.h"
#include "../sync/bvmm_sync.h"
#include "kernel/drivers/display/display.h"

/**
 * @file bvmm_phase7_tests.c
 * @brief Production Certification Tests for BVMM Phase 7 (Production Fence & Synchronization Engine BFSE V1.0)
 */

bool bvmm_run_phase7_tests(void) {
    display_print("[BVMM TEST] Starting Phase 7 Production Fence & Synchronization Engine (BFSE) Certification Suite...\n");

    if (!bvmm_is_initialized()) {
        bvmm_init();
    }

    /* 1. Monotonic Timeline Creation & Increment Test */
    bfse_timeline_id_t timeline_id = BFSE_INVALID_TIMELINE_ID;
    bvmm_result_t res = bfse_timeline_create(1, &timeline_id);
    if (res != BVMM_SUCCESS || timeline_id == BFSE_INVALID_TIMELINE_ID) {
        display_print("[BVMM TEST] FAIL: Timeline creation failed\n");
        return false;
    }

    res = bfse_timeline_advance(timeline_id, 100);
    uint64_t val = 0;
    if (res != BVMM_SUCCESS || bfse_timeline_query(timeline_id, &val) != BVMM_SUCCESS || val != 100) {
        display_print("[BVMM TEST] FAIL: Timeline advance/query failed\n");
        return false;
    }

    /* Rejection of non-monotonic advance */
    if (bfse_timeline_advance(timeline_id, 50) == BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Non-monotonic timeline decrease was not rejected\n");
        return false;
    }

    /* 2. Fence Creation, Wait & Signal Test */
    bfse_fence_id_t fence_id = BFSE_INVALID_FENCE_ID;
    res = bfse_fence_create(timeline_id, 200, BFSE_QUEUE_GRAPHICS, &fence_id);
    if (res != BVMM_SUCCESS || fence_id == BFSE_INVALID_FENCE_ID) {
        display_print("[BVMM TEST] FAIL: Fence creation failed\n");
        return false;
    }

    /* Wait should return pending/timeout because timeline is 100 < 200 */
    if (bfse_wait(fence_id, 0) == BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Unsignaled fence wait succeeded prematurely\n");
        return false;
    }

    /* Signal fence advances timeline to 200 */
    res = bfse_signal(fence_id);
    if (res != BVMM_SUCCESS || bfse_wait(fence_id, 0) != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Fence signal or post-signal wait failed\n");
        return false;
    }

    /* 3. Wait Any & Wait All Primitives Test */
    bfse_fence_id_t f1 = BFSE_INVALID_FENCE_ID;
    bfse_fence_id_t f2 = BFSE_INVALID_FENCE_ID;
    bfse_fence_create(timeline_id, 300, BFSE_QUEUE_COPY, &f1);
    bfse_fence_create(timeline_id, 400, BFSE_QUEUE_UPLOAD, &f2);

    bfse_fence_id_t fence_batch[2] = { f1, f2 };
    bfse_signal(f1);

    if (bfse_wait_any(fence_batch, 2, 0) != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Wait Any primitive failed\n");
        return false;
    }

    if (bfse_wait_all(fence_batch, 2, 0) == BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Wait All primitive succeeded prematurely before f2 was signaled\n");
        return false;
    }

    bfse_signal(f2);
    if (bfse_wait_all(fence_batch, 2, 0) != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Wait All primitive failed after all fences signaled\n");
        return false;
    }

    /* 4. Barrier Pipeline Test */
    res = bfse_barrier_emit(BFSE_BARRIER_TEXTURE, BFSE_QUEUE_GRAPHICS);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Barrier emission failed\n");
        return false;
    }

    /* 5. Deadlock Detection Test */
    bfse_deadlock_report_t report;
    res = bfse_detect_deadlocks(&report);
    if (res != BVMM_SUCCESS || report.deadlock_detected) {
        display_print("[BVMM TEST] FAIL: False deadlock detected\n");
        return false;
    }

    bfse_fence_destroy(f1);
    bfse_fence_destroy(f2);
    bfse_fence_destroy(fence_id);

    /* 6. 100,000 Fence Operations & Recycling Stress Test */
    display_print("[BVMM TEST] Executing 100,000 Fence Operations & Recycling Stress Test...\n");
    for (int i = 0; i < 100000; i++) {
        bfse_fence_id_t stress_fence = BFSE_INVALID_FENCE_ID;
        res = bfse_fence_create(timeline_id, (uint64_t)(500 + i), BFSE_QUEUE_GRAPHICS, &stress_fence);
        if (res != BVMM_SUCCESS || stress_fence == BFSE_INVALID_FENCE_ID) {
            display_print("[BVMM TEST] FAIL: 100,000 fence stress creation failed at iteration ");
            display_print_dec((uint32_t)i);
            display_print("\n");
            return false;
        }
        bfse_signal(stress_fence);
        bfse_fence_destroy(stress_fence); /* Recycles descriptor */
    }
    display_print("[BVMM TEST] 100,000 Fence Operations Stress Test: PASS\n");

    /* 7. Diagnostics & Inspector Dump */
    bfse_fence_dump_all();

    display_print("[BVMM TEST] PASS: All Phase 7 Production Fence & Synchronization Engine Certification Tests Passed!\n");
    return true;
}
