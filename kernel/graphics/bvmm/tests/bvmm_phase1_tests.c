#include "../include/bvmm.h"
#include "kernel/drivers/display/display.h"

/**
 * @file bvmm_phase1_tests.c
 * @brief Certification Tests for BVMM Phase 1 (Core Foundation)
 */

bool bvmm_run_phase1_tests(void) {
    display_print("[BVMM TEST] Starting Phase 1 Core Foundation Tests...\n");

    /* 1. Handle Encoding / Decoding Macros Test */
    uint16_t gen = 0x1234;
    uint16_t flags = 0x5678;
    uint32_t index = 0x9ABCDEF0;
    bvmm_handle_t handle = BVMM_MAKE_HANDLE(gen, flags, index);

    if (BVMM_HANDLE_GET_GEN(handle) != gen) {
        display_print("[BVMM TEST] FAIL: Handle generation ID mismatch\n");
        return false;
    }
    if (BVMM_HANDLE_GET_FLAGS(handle) != flags) {
        display_print("[BVMM TEST] FAIL: Handle flags mismatch\n");
        return false;
    }
    if (BVMM_HANDLE_GET_INDEX(handle) != index) {
        display_print("[BVMM TEST] FAIL: Handle index mismatch\n");
        return false;
    }

    /* 2. Initialization & State Transition Test */
    if (bvmm_is_initialized()) {
        display_print("[BVMM TEST] FAIL: BVMM should not be initialized before bvmm_init()\n");
        return false;
    }

    bvmm_result_t res = bvmm_init();
    if (res != BVMM_SUCCESS || !bvmm_is_initialized()) {
        display_print("[BVMM TEST] FAIL: bvmm_init() failed\n");
        return false;
    }

    /* 3. Double Initialization Guard Test */
    res = bvmm_init();
    if (res != BVMM_ERR_ALREADY_INITIALIZED) {
        display_print("[BVMM TEST] FAIL: Double initialization was not blocked\n");
        return false;
    }

    /* 4. Statistics Retrieval Test */
    bvmm_stats_t stats;
    res = bvmm_get_stats(&stats);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Failed to query BVMM statistics\n");
        return false;
    }

    /* 5. Clean Shutdown Test */
    res = bvmm_shutdown();
    if (res != BVMM_SUCCESS || bvmm_is_initialized()) {
        display_print("[BVMM TEST] FAIL: bvmm_shutdown() failed\n");
        return false;
    }

    display_print("[BVMM TEST] PASS: All Phase 1 Core Foundation Tests Passed!\n");
    return true;
}
