#include "../include/bvmm.h"
#include "../heap/bvmm_heap.h"
#include "../diagnostics/bvmm_stats.h"
#include "kernel/drivers/display/display.h"

/**
 * @file bvmm_phase2_tests.c
 * @brief Production Certification Tests for BVMM Phase 2 (VRAM Heap Manager & Hybrid Engine)
 */

bool bvmm_run_phase2_tests(void) {
    display_print("[BVMM TEST] Starting Phase 2 VRAM Heap Manager Certification Suite...\n");

    if (!bvmm_is_initialized()) {
        bvmm_init();
    }

    /* 1. Small Allocation Test (<64KB Cursor Plane) */
    bvmm_alloc_info_t small_info = {
        .size_bytes       = 16 * 1024, /* 16 KB */
        .alignment_bytes  = 4096,      /* 4KB Page Alignment */
        .preferred_domain = BVMM_DOMAIN_VRAM,
        .pool_type        = BVMM_POOL_SMALL,
        .alloc_flags      = BVMM_ALLOC_FLAG_PINNED,
        .owner_pid        = 1
    };

    bvmm_handle_t h_small = BVMM_INVALID_HANDLE;
    bvmm_result_t res = bvmm_allocate(&small_info, &h_small);
    if (res != BVMM_SUCCESS || h_small == BVMM_INVALID_HANDLE) {
        display_print("[BVMM TEST] FAIL: Small allocation failed\n");
        return false;
    }

    /* 2. Large Allocation Test (>64MB 4K Scanout Buffer) */
    bvmm_alloc_info_t large_info = {
        .size_bytes       = 68 * 1024 * 1024, /* 68 MB */
        .alignment_bytes  = 65536,            /* 64KB Alignment */
        .preferred_domain = BVMM_DOMAIN_VRAM,
        .pool_type        = BVMM_POOL_HUGE,
        .alloc_flags      = BVMM_ALLOC_FLAG_SCANOUT,
        .owner_pid        = 1,
        .width            = 3840,
        .height           = 2160,
        .stride_bytes     = 15360,
        .format_fourcc    = 0x34325241 /* ARGB8888 */
    };

    bvmm_handle_t h_large = BVMM_INVALID_HANDLE;
    res = bvmm_allocate(&large_info, &h_large);
    if (res != BVMM_SUCCESS || h_large == BVMM_INVALID_HANDLE) {
        display_print("[BVMM TEST] FAIL: Large allocation failed\n");
        return false;
    }

    /* 3. Alignment Correctness Verification */
    void* cpu_ptr = NULL;
    res = bvmm_map(h_large, &cpu_ptr);
    if (res != BVMM_SUCCESS || ((uintptr_t)cpu_ptr & 65535) != 0) {
        display_print("[BVMM TEST] FAIL: 64KB alignment check failed\n");
        return false;
    }

    /* 4. Query Info Test */
    bvmm_alloc_info_t queried_info;
    res = bvmm_get_info(h_large, &queried_info);
    if (res != BVMM_SUCCESS || queried_info.size_bytes != large_info.size_bytes) {
        display_print("[BVMM TEST] FAIL: Allocation descriptor query mismatch\n");
        return false;
    }

    /* 5. 100,000+ Allocation & Free Stress Test */
    display_print("[BVMM TEST] Executing 100,000 Cycle Allocation Stress Test...\n");
    bvmm_alloc_info_t stress_info = {
        .size_bytes       = 4096,
        .alignment_bytes  = 256,
        .preferred_domain = BVMM_DOMAIN_VRAM,
        .pool_type        = BVMM_POOL_MEDIUM,
        .owner_pid        = 1
    };

    for (int i = 0; i < 100000; i++) {
        bvmm_handle_t h_stress = BVMM_INVALID_HANDLE;
        res = bvmm_allocate(&stress_info, &h_stress);
        if (res != BVMM_SUCCESS) {
            display_print("[BVMM TEST] FAIL: Stress test allocation failed at cycle ");
            display_print_dec((uint32_t)i);
            display_print("\n");
            return false;
        }
        bvmm_free(h_stress);
    }
    display_print("[BVMM TEST] 100,000 Cycle Allocation Stress Test: PASS\n");

    /* 6. Coalescing & Recovery Verification */
    bvmm_free(h_small);
    bvmm_free(h_large);

    /* 7. Real-Time Telemetry & Fragmentation Analysis */
    bvmm_frag_analysis_t frag;
    res = bvmm_stats_analyze_fragmentation(&frag);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Fragmentation analysis failed\n");
        return false;
    }

    bvmm_stats_t stats;
    res = bvmm_get_stats(&stats);
    if (res != BVMM_SUCCESS || stats.used_vram_bytes != 0) {
        display_print("[BVMM TEST] FAIL: Telemetry free recovery mismatch\n");
        return false;
    }

    display_print("[BVMM TEST] PASS: All Phase 2 VRAM Heap Manager Certification Tests Passed!\n");
    return true;
}
