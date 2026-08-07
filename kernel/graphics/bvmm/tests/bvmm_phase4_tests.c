#include "../include/bvmm.h"
#include "../surface/bvmm_surface.h"
#include "kernel/drivers/display/display.h"

/**
 * @file bvmm_phase4_tests.c
 * @brief Production Certification Tests for BVMM Phase 4 (BOSurface Manager Engine BSME V1.0)
 */

bool bvmm_run_phase4_tests(void) {
    display_print("[BVMM TEST] Starting Phase 4 BOSurface Manager Engine Certification Suite...\n");

    if (!bvmm_is_initialized()) {
        bvmm_init();
    }

    /* 1. Single Surface Creation Test */
    bvmm_surface_create_info_t create_info = {
        .width            = 1920,
        .height           = 1080,
        .format           = BVMM_SURF_FMT_ARGB8888,
        .flags            = BVMM_SURF_FLAG_RENDER_TARGET | BVMM_SURF_FLAG_CPU_VISIBLE,
        .preferred_domain = BVMM_DOMAIN_VRAM,
        .lifetime         = BVMM_LIFETIME_WINDOW,
        .owner_pid        = 1,
        .owner_module     = 101
    };

    bvmm_surface_id_t s_id = BVMM_INVALID_SURFACE_ID;
    bvmm_result_t res = bvmm_surface_create(&create_info, &s_id);
    if (res != BVMM_SUCCESS || s_id == BVMM_INVALID_SURFACE_ID) {
        display_print("[BVMM TEST] FAIL: Single surface creation failed\n");
        return false;
    }

    /* 2. Lookup & Descriptor Verification */
    bvmm_surface_desc_t* desc = NULL;
    res = bvmm_surface_lookup(s_id, &desc);
    if (res != BVMM_SUCCESS || !desc || desc->width != 1920 || desc->height != 1080) {
        display_print("[BVMM TEST] FAIL: Surface lookup verification failed\n");
        return false;
    }

    /* 3. Surface Lock & Unlock Test */
    void* cpu_ptr = NULL;
    res = bvmm_surface_lock(s_id, &cpu_ptr);
    if (res != BVMM_SUCCESS || !cpu_ptr) {
        display_print("[BVMM TEST] FAIL: Surface lock failed\n");
        return false;
    }

    res = bvmm_surface_unlock(s_id);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Surface unlock failed\n");
        return false;
    }

    /* Illegal Double Unlock Rejection Test */
    res = bvmm_surface_unlock(s_id);
    if (res == BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Illegal double unlock was not rejected\n");
        return false;
    }

    /* 4. Surface Cloning Test */
    bvmm_surface_id_t cloned_id = BVMM_INVALID_SURFACE_ID;
    res = bvmm_surface_clone(s_id, &cloned_id);
    if (res != BVMM_SUCCESS || cloned_id == BVMM_INVALID_SURFACE_ID) {
        display_print("[BVMM TEST] FAIL: Surface cloning failed\n");
        return false;
    }

    /* 5. Live Surface Resizing Test */
    res = bvmm_surface_resize(cloned_id, 2560, 1440);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Live surface resize failed\n");
        return false;
    }

    /* 6. Reference Counting & Auto Zero-Ref Cleanup Test */
    bvmm_surface_ref_inc(s_id);
    if (desc->ref_count != 2) {
        display_print("[BVMM TEST] FAIL: Reference increment failed\n");
        return false;
    }
    bvmm_surface_ref_dec(s_id);
    bvmm_surface_ref_dec(s_id); /* RefCount becomes 0 -> Auto-destroyed */

    /* Lookup should now fail */
    bvmm_surface_desc_t* dead_desc = NULL;
    if (bvmm_surface_lookup(s_id, &dead_desc) == BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Surface auto zero-ref destruction failed\n");
        return false;
    }

    bvmm_surface_destroy(cloned_id);

    /* 7. 1000 Surface Creation & Destruction Stress Loop */
    display_print("[BVMM TEST] Executing 1,000 Surface Creation & Destruction Stress Test...\n");
    bvmm_surface_create_info_t stress_info = {
        .width            = 64,
        .height           = 64,
        .format           = BVMM_SURF_FMT_RGBA8888,
        .flags            = BVMM_SURF_FLAG_STATIC,
        .owner_pid        = 1
    };

    for (int i = 0; i < 1000; i++) {
        bvmm_surface_id_t stress_id = BVMM_INVALID_SURFACE_ID;
        res = bvmm_surface_create(&stress_info, &stress_id);
        if (res != BVMM_SUCCESS || stress_id == BVMM_INVALID_SURFACE_ID) {
            display_print("[BVMM TEST] FAIL: 1,000 surface stress test creation failed at iteration ");
            display_print_dec((uint32_t)i);
            display_print("\n");
            return false;
        }
        bvmm_surface_destroy(stress_id);
    }
    display_print("[BVMM TEST] 1,000 Surface Stress Test: PASS\n");

    /* 8. Diagnostics & Inspector Dump */
    bvmm_surface_dump_all();

    display_print("[BVMM TEST] PASS: All Phase 4 BOSurface Manager Engine Certification Tests Passed!\n");
    return true;
}
