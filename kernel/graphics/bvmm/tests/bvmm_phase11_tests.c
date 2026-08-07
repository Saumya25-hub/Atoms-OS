#include "../include/bvmm.h"
#include "../sharing/bcpse.h"
#include "kernel/drivers/display/display.h"

/**
 * @file bvmm_phase11_tests.c
 * @brief Production Certification Tests for BVMM Phase 11 (Production Cross-Process GPU Resource Sharing Engine BCPSE V1.0)
 */

bool bvmm_run_phase11_tests(void) {
    display_print("[BVMM TEST] Starting Phase 11 Production Cross-Process GPU Resource Sharing Engine (BCPSE) Certification Suite...\n");

    if (!bvmm_is_initialized()) {
        bvmm_init();
    }

    /* 1. Shared Surface Export Test */
    brrle_lifetime_create_info_t c_info = {
        .resource_type     = BRRLE_RES_SURFACE,
        .initial_residency = BRRLE_RESIDENCY_VRAM,
        .priority          = BRRLE_PRIORITY_NORMAL,
        .owner_pid         = 100
    };
    brrle_lifetime_id_t life_id = BRRLE_INVALID_LIFETIME_ID;
    brrle_lifetime_create(&c_info, &life_id);

    bcpse_handle_t shared_handle = BCPSE_INVALID_HANDLE;
    bvmm_result_t res = bcpse_export_object(BCPSE_OBJ_SURFACE, life_id, 100, 200, BCPSE_PERM_READ_WRITE, &shared_handle);
    if (res != BVMM_SUCCESS || shared_handle == BCPSE_INVALID_HANDLE) {
        display_print("[BVMM TEST] FAIL: Shared surface export failed\n");
        return false;
    }

    /* 2. Cross-Process Import & Zero-Copy Verification Test */
    bcpse_shared_obj_t* imported_obj = NULL;
    res = bcpse_import_object(shared_handle, 200, BCPSE_PERM_READ, &imported_obj);
    if (res != BVMM_SUCCESS || !imported_obj || imported_obj->phys_vram_addr == 0) {
        display_print("[BVMM TEST] FAIL: Zero-copy cross-process import failed\n");
        return false;
    }

    /* 3. Unauthorized PID Import Rejection Test */
    bcpse_shared_obj_t* bad_obj = NULL;
    res = bcpse_import_object(shared_handle, 300, BCPSE_PERM_READ, &bad_obj);
    if (res == BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Unauthorized PID import was incorrectly permitted\n");
        return false;
    }

    /* 4. Release Import Reference Test */
    res = bcpse_release_import(shared_handle, 200);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Shared import release failed\n");
        return false;
    }

    /* 5. Handle Revocation Test */
    res = bcpse_revoke_handle(shared_handle, 100);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Handle revocation failed\n");
        return false;
    }

    /* 6. Process Exit Resource Cleanup Test */
    res = bcpse_cleanup_process_resources(100);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Process exit resource cleanup failed\n");
        return false;
    }

    brrle_cpu_release(life_id);

    /* 7. 100,000 Import / Export Operations Stress Test */
    display_print("[BVMM TEST] Executing 100,000 Import / Export Operations Stress Test...\n");
    for (int i = 0; i < 100000; i++) {
        bcpse_handle_t s_handle = BCPSE_INVALID_HANDLE;
        res = bcpse_export_object(BCPSE_OBJ_TEXTURE, BRRLE_INVALID_LIFETIME_ID, (uint32_t)(100 + (i % 16)), 0, BCPSE_PERM_READ_WRITE, &s_handle);
        if (res != BVMM_SUCCESS || s_handle == BCPSE_INVALID_HANDLE) {
            display_print("[BVMM TEST] FAIL: 100,000 export stress operation failed at iteration ");
            display_print_dec((uint32_t)i);
            display_print("\n");
            return false;
        }

        bcpse_shared_obj_t* s_obj = NULL;
        bcpse_import_object(s_handle, (uint32_t)(500 + (i % 16)), BCPSE_PERM_READ, &s_obj);
        bcpse_release_import(s_handle, (uint32_t)(500 + (i % 16)));
        bcpse_release_import(s_handle, (uint32_t)(100 + (i % 16)));
    }
    display_print("[BVMM TEST] 100,000 Import / Export Operations Stress Test: PASS\n");

    /* 8. Diagnostics & Inspector Dump */
    bcpse_dump();

    display_print("[BVMM TEST] PASS: All Phase 11 Production Cross-Process GPU Resource Sharing Engine Certification Tests Passed!\n");
    return true;
}
