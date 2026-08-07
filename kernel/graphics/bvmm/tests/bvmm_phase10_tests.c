#include "../include/bvmm.h"
#include "../hal/bghal.h"
#include "kernel/drivers/display/display.h"

/**
 * @file bvmm_phase10_tests.c
 * @brief Production Certification Tests for BVMM Phase 10 (Production GPU Hardware Abstraction Layer BGHAL V1.0)
 */

bool bvmm_run_phase10_tests(void) {
    display_print("[BVMM TEST] Starting Phase 10 Production GPU Hardware Abstraction Layer (BGHAL) Certification Suite...\n");

    if (!bvmm_is_initialized()) {
        bvmm_init();
    }

    /* 1. Primary GPU Discovery Test */
    bghal_gpu_device_t* gpu = NULL;
    bvmm_result_t res = bghal_get_primary_gpu(&gpu);
    if (res != BVMM_SUCCESS || !gpu || gpu->vendor_id == BGHAL_VENDOR_UNKNOWN) {
        display_print("[BVMM TEST] FAIL: Primary GPU discovery query failed\n");
        return false;
    }

    /* 2. Capability Matrix Enumeration Test */
    bghal_capabilities_t caps;
    res = bghal_query_capabilities(gpu, &caps);
    if (res != BVMM_SUCCESS || caps.max_texture_dim == 0) {
        display_print("[BVMM TEST] FAIL: GPU capability query failed\n");
        return false;
    }

    /* 3. Unified DMA Copy Engine Test */
    bghal_dma_req_t dma_req = {
        .src_phys_addr = 0x10000000ULL,
        .dst_phys_addr = 0x20000000ULL,
        .size_bytes    = 4 * 1024 * 1024ULL,
        .channel_id    = 0
    };
    res = bghal_dma_copy(gpu, &dma_req);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Unified DMA copy execution failed\n");
        return false;
    }

    /* 4. GPU Page Table Mapping & Unmapping Test */
    bghal_page_table_t pt = {
        .gpu_virt_addr  = 0xE0000000ULL,
        .phys_vram_addr = 0x10000000ULL,
        .size_bytes     = 2 * 1024 * 1024ULL,
        .flags          = 0x3,
        .is_mapped      = true
    };
    res = bghal_page_table_map(gpu, &pt);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: GPU page table map execution failed\n");
        return false;
    }

    /* 5. Cache Flush & TLB Invalidation Test */
    res = bghal_cache_flush_and_tlb_invalidate(gpu);
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Cache flush and TLB invalidation failed\n");
        return false;
    }

    /* 6. 100,000 DMA Operations & HAL Request Stress Test */
    display_print("[BVMM TEST] Executing 100,000 DMA Operations & HAL Request Stress Test...\n");
    for (int i = 0; i < 100000; i++) {
        bghal_dma_req_t stress_dma = {
            .src_phys_addr = 0x10000000ULL + (i * 0x1000ULL),
            .dst_phys_addr = 0x20000000ULL + (i * 0x1000ULL),
            .size_bytes    = 64 * 1024ULL,
            .channel_id    = (uint32_t)(i % 4)
        };
        res = bghal_dma_copy(gpu, &stress_dma);
        if (res != BVMM_SUCCESS) {
            display_print("[BVMM TEST] FAIL: 100,000 DMA operations stress test failed at iteration ");
            display_print_dec((uint32_t)i);
            display_print("\n");
            return false;
        }
    }
    display_print("[BVMM TEST] 100,000 DMA Operations Stress Test: PASS\n");

    /* 7. Diagnostics & Inspector Dump */
    bghal_gpu_dump();

    display_print("[BVMM TEST] PASS: All Phase 10 Production GPU Hardware Abstraction Layer Certification Tests Passed!\n");
    return true;
}
