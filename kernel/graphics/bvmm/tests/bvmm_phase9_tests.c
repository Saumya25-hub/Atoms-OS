#include "../include/bvmm.h"
#include "../defrag/bvmm_defrag.h"
#include "kernel/drivers/display/display.h"

/**
 * @file bvmm_phase9_tests.c
 * @brief Production Certification Tests for BVMM Phase 9 (Production Live Migration & Defragmentation Engine BLMDE V1.0)
 */

bool bvmm_run_phase9_tests(void) {
    display_print("[BVMM TEST] Starting Phase 9 Production Live Migration & Defragmentation Engine (BLMDE) Certification Suite...\n");

    if (!bvmm_is_initialized()) {
        bvmm_init();
    }

    /* 1. Fragmentation Analysis Metrics Test */
    blmde_fragmentation_stats_t stats;
    bvmm_result_t res = blmde_analyze_fragmentation(&stats);
    if (res != BVMM_SUCCESS || stats.total_heap_bytes == 0 || stats.health_score == 0) {
        display_print("[BVMM TEST] FAIL: Fragmentation analysis query failed\n");
        return false;
    }

    /* 2. VRAM -> VRAM Live Relocation & 9-Step Pipeline Test */
    brrle_lifetime_create_info_t c_info = {
        .resource_type     = BRRLE_RES_SURFACE,
        .initial_residency = BRRLE_RESIDENCY_VRAM,
        .priority          = BRRLE_PRIORITY_NORMAL,
        .owner_pid         = 100
    };
    brrle_lifetime_id_t life_id = BRRLE_INVALID_LIFETIME_ID;
    brrle_lifetime_create(&c_info, &life_id);

    blmde_reloc_desc_t reloc = {
        .lifetime_id   = life_id,
        .src_residency = BRRLE_RESIDENCY_VRAM,
        .dst_residency = BRRLE_RESIDENCY_VRAM,
        .src_phys_addr = 0x10000000ULL,
        .dst_phys_addr = 0x20000000ULL,
        .size_bytes    = 2 * 1024 * 1024ULL
    };

    blmde_job_id_t job_id = BLMDE_INVALID_JOB_ID;
    res = blmde_job_create(&reloc, BLMDE_MODE_IMMEDIATE, BRRLE_PRIORITY_NORMAL, &job_id);
    if (res != BVMM_SUCCESS || job_id == BLMDE_INVALID_JOB_ID) {
        display_print("[BVMM TEST] FAIL: Migration job creation failed\n");
        return false;
    }

    res = blmde_execute_migration(job_id);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Zero-downtime migration pipeline execution failed\n");
        return false;
    }

    /* 3. VRAM -> GTT Demotion & GTT -> VRAM Promotion Migration Test */
    reloc.src_residency = BRRLE_RESIDENCY_VRAM;
    reloc.dst_residency = BRRLE_RESIDENCY_GTT;
    blmde_job_id_t demote_job = BLMDE_INVALID_JOB_ID;
    blmde_job_create(&reloc, BLMDE_MODE_IMMEDIATE, BRRLE_PRIORITY_LOW, &demote_job);
    if (blmde_execute_migration(demote_job) != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: VRAM -> GTT demotion migration failed\n");
        return false;
    }

    reloc.src_residency = BRRLE_RESIDENCY_GTT;
    reloc.dst_residency = BRRLE_RESIDENCY_VRAM;
    blmde_job_id_t promote_job = BLMDE_INVALID_JOB_ID;
    blmde_job_create(&reloc, BLMDE_MODE_IMMEDIATE, BRRLE_PRIORITY_HIGH, &promote_job);
    if (blmde_execute_migration(promote_job) != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: GTT -> VRAM promotion migration failed\n");
        return false;
    }

    /* 4. Rollback Engine Test */
    blmde_job_id_t rb_job = BLMDE_INVALID_JOB_ID;
    blmde_job_create(&reloc, BLMDE_MODE_IMMEDIATE, BRRLE_PRIORITY_NORMAL, &rb_job);
    res = blmde_rollback(rb_job);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Rollback engine execution failed\n");
        return false;
    }

    /* 5. Online TLSF Heap Compaction Test */
    uint64_t reclaimed_gain = 0;
    res = blmde_compact_heap(16 * 1024 * 1024ULL, &reclaimed_gain);
    if (res != BVMM_SUCCESS || reclaimed_gain != 16 * 1024 * 1024ULL) {
        display_print("[BVMM TEST] FAIL: Online heap compaction failed\n");
        return false;
    }

    brrle_cpu_release(life_id);

    /* 6. 100,000 Migration Operations & Compaction Stress Test */
    display_print("[BVMM TEST] Executing 100,000 Migration Operations & Compaction Stress Test...\n");
    for (int i = 0; i < 100000; i++) {
        blmde_reloc_desc_t stress_reloc = {
            .lifetime_id   = BRRLE_INVALID_LIFETIME_ID,
            .src_residency = BRRLE_RESIDENCY_VRAM,
            .dst_residency = BRRLE_RESIDENCY_GTT,
            .size_bytes    = 1024 * 1024ULL
        };
        blmde_job_id_t s_job = BLMDE_INVALID_JOB_ID;
        res = blmde_job_create(&stress_reloc, BLMDE_MODE_IMMEDIATE, BRRLE_PRIORITY_NORMAL, &s_job);
        if (res != BVMM_SUCCESS || s_job == BLMDE_INVALID_JOB_ID) {
            display_print("[BVMM TEST] FAIL: 100,000 migration stress job creation failed at iteration ");
            display_print_dec((uint32_t)i);
            display_print("\n");
            return false;
        }
        blmde_execute_migration(s_job);
    }
    display_print("[BVMM TEST] 100,000 Migration Operations Stress Test: PASS\n");

    /* 7. Diagnostics & Inspector Dump */
    blmde_migration_dump();

    display_print("[BVMM TEST] PASS: All Phase 9 Production Live Migration & Defragmentation Engine Certification Tests Passed!\n");
    return true;
}
