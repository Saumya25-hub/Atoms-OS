#include "../include/bvmm.h"
#include "../pools/bvmm_pools.h"
#include "../diagnostics/bvmm_stats.h"
#include "kernel/drivers/display/display.h"

/**
 * @file bvmm_phase3_tests.c
 * @brief Production Certification Tests for BVMM Phase 3 (Production Memory Pool Engine)
 */

bool bvmm_run_phase3_tests(void) {
    display_print("[BVMM TEST] Starting Phase 3 Production Memory Pool Engine Certification Suite...\n");

    if (!bvmm_is_initialized()) {
        bvmm_init();
    }

    /* 1. Small Pool Test (<64KB Cursor Object) */
    bvmm_alloc_info_t small_info = {
        .size_bytes       = 4096,
        .alignment_bytes  = 16,
        .preferred_domain = BVMM_DOMAIN_VRAM,
        .pool_type        = BVMM_POOL_SMALL,
        .owner_pid        = 1
    };
    bvmm_handle_t h_small = BVMM_INVALID_HANDLE;
    bvmm_result_t res = bvmm_allocate(&small_info, &h_small);
    if (res != BVMM_SUCCESS || h_small == BVMM_INVALID_HANDLE) {
        display_print("[BVMM TEST] FAIL: Small Pool allocation failed\n");
        return false;
    }

    /* 2. Medium Pool Test (1MB Glyph Atlas Texture) */
    bvmm_alloc_info_t medium_info = {
        .size_bytes       = 1024 * 1024,
        .alignment_bytes  = 256,
        .preferred_domain = BVMM_DOMAIN_VRAM,
        .pool_type        = BVMM_POOL_MEDIUM,
        .owner_pid        = 1
    };
    bvmm_handle_t h_medium = BVMM_INVALID_HANDLE;
    res = bvmm_allocate(&medium_info, &h_medium);
    if (res != BVMM_SUCCESS || h_medium == BVMM_INVALID_HANDLE) {
        display_print("[BVMM TEST] FAIL: Medium Pool allocation failed\n");
        return false;
    }

    /* 3. Large Pool Test (16MB Window Render Target) */
    bvmm_alloc_info_t large_info = {
        .size_bytes       = 16 * 1024 * 1024,
        .alignment_bytes  = 65536,
        .preferred_domain = BVMM_DOMAIN_VRAM,
        .pool_type        = BVMM_POOL_LARGE,
        .owner_pid        = 1
    };
    bvmm_handle_t h_large = BVMM_INVALID_HANDLE;
    res = bvmm_allocate(&large_info, &h_large);
    if (res != BVMM_SUCCESS || h_large == BVMM_INVALID_HANDLE) {
        display_print("[BVMM TEST] FAIL: Large Pool allocation failed\n");
        return false;
    }

    /* 4. Huge Pool Test (68MB 4K HDR Scanout) */
    bvmm_alloc_info_t huge_info = {
        .size_bytes       = 68 * 1024 * 1024,
        .alignment_bytes  = 65536,
        .preferred_domain = BVMM_DOMAIN_VRAM,
        .pool_type        = BVMM_POOL_HUGE,
        .alloc_flags      = BVMM_POOL_FLAG_SCANOUT,
        .owner_pid        = 1
    };
    bvmm_handle_t h_huge = BVMM_INVALID_HANDLE;
    res = bvmm_allocate(&huge_info, &h_huge);
    if (res != BVMM_SUCCESS || h_huge == BVMM_INVALID_HANDLE) {
        display_print("[BVMM TEST] FAIL: Huge Pool allocation failed\n");
        return false;
    }

    /* 5. Transient Pool & O(1) Bulk Frame Reset Test */
    bvmm_alloc_info_t trans_info = {
        .size_bytes       = 64 * 1024,
        .alignment_bytes  = 16,
        .alloc_flags      = BVMM_POOL_FLAG_TRANSIENT,
        .owner_pid        = 1
    };
    bvmm_handle_t h_trans = BVMM_INVALID_HANDLE;
    res = bvmm_allocate(&trans_info, &h_trans);
    if (res != BVMM_SUCCESS || h_trans == BVMM_INVALID_HANDLE) {
        display_print("[BVMM TEST] FAIL: Transient Pool allocation failed\n");
        return false;
    }

    res = bvmm_pool_transient_reset();
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Transient pool O(1) bulk reset failed\n");
        return false;
    }

    /* 6. Upload & Readback Staging Pool Tests */
    bvmm_alloc_info_t upload_info = {
        .size_bytes  = 256 * 1024,
        .alloc_flags = BVMM_POOL_FLAG_UPLOAD,
        .owner_pid   = 1
    };
    bvmm_handle_t h_upload = BVMM_INVALID_HANDLE;
    res = bvmm_allocate(&upload_info, &h_upload);
    if (res != BVMM_SUCCESS || h_upload == BVMM_INVALID_HANDLE) {
        display_print("[BVMM TEST] FAIL: Upload Pool allocation failed\n");
        return false;
    }

    /* 7. Automatic Pool Routing Verification */
    bvmm_alloc_info_t query_info;
    res = bvmm_get_info(h_upload, &query_info);
    if (res != BVMM_SUCCESS || query_info.pool_type != BVMM_POOL_UPLOAD) {
        display_print("[BVMM TEST] FAIL: Automatic pool routing mismatch\n");
        return false;
    }

    /* 8. 500,000 Cycle Allocation & Free Stress Test */
    display_print("[BVMM TEST] Executing 500,000 Cycle Pool Allocation Stress Test...\n");
    bvmm_alloc_info_t stress_info = {
        .size_bytes       = 8192,
        .alignment_bytes  = 256,
        .preferred_domain = BVMM_DOMAIN_VRAM,
        .pool_type        = BVMM_POOL_MEDIUM,
        .owner_pid        = 1
    };

    for (int i = 0; i < 500000; i++) {
        bvmm_handle_t h_stress = BVMM_INVALID_HANDLE;
        res = bvmm_allocate(&stress_info, &h_stress);
        if (res != BVMM_SUCCESS) {
            display_print("[BVMM TEST] FAIL: 500,000 stress test allocation failed at cycle ");
            display_print_dec((uint32_t)i);
            display_print("\n");
            return false;
        }
        bvmm_free(h_stress);
    }
    display_print("[BVMM TEST] 500,000 Cycle Pool Allocation Stress Test: PASS\n");

    /* Clean up remaining test allocations */
    bvmm_free(h_small);
    bvmm_free(h_medium);
    bvmm_free(h_large);
    bvmm_free(h_huge);
    bvmm_free(h_upload);

    /* 9. DPDP Per-Pool Telemetry Dump */
    bvmm_stats_dump_pool_forensics();

    display_print("[BVMM TEST] PASS: All Phase 3 Production Memory Pool Engine Certification Tests Passed!\n");
    return true;
}
