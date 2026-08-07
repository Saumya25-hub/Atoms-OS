#include "../include/bvmm_api.h"
#include "../heap/bvmm_heap.h"
#include "../pools/bvmm_pools.h"
#include "../surface/bvmm_surface.h"
#include "../texture/bvmm_texture.h"
#include "../lifetime/bvmm_lifetime.h"
#include "../sync/bvmm_sync.h"
#include "../policy/bvmm_policy.h"
#include "../defrag/bvmm_defrag.h"
#include "../hal/bghal.h"
#include "../sharing/bcpse.h"
#include "../optimization/bpoe.h"
#include "../handles/bvmm_handle_table.h"
#include "../diagnostics/bvmm_stats.h"
#include "kernel/drivers/display/display.h"

static bool g_bvmm_initialized = false;

bvmm_result_t bvmm_init(void) {
    if (g_bvmm_initialized) return BVMM_ERR_ALREADY_INITIALIZED;

    /* 1. Initialize Handle Table Manager */
    bvmm_result_t res = bvmm_handle_table_init();
    if (res != BVMM_SUCCESS) return res;

    /* 2. Initialize Telemetry & Diagnostics Engine */
    res = bvmm_stats_init();
    if (res != BVMM_SUCCESS) {
        bvmm_handle_table_shutdown();
        return res;
    }

    /* 3. Initialize VRAM Heap Manager Subsystem & Classified Heaps */
    res = bvmm_heap_subsystem_init();
    if (res != BVMM_SUCCESS) {
        bvmm_handle_table_shutdown();
        return res;
    }

    /* 4. Initialize Production Memory Pool Engine (PMPE) */
    res = bvmm_pools_init();
    if (res != BVMM_SUCCESS) {
        bvmm_heap_subsystem_shutdown();
        bvmm_handle_table_shutdown();
        return res;
    }

    /* 5. Initialize BOSurface Manager Engine (BSME V1.0) */
    res = bvmm_surface_engine_init();
    if (res != BVMM_SUCCESS) {
        bvmm_pools_shutdown();
        bvmm_heap_subsystem_shutdown();
        bvmm_handle_table_shutdown();
        return res;
    }

    /* 6. Initialize Production Texture & Format Engine (BTFE V1.0) */
    res = btfe_init();
    if (res != BVMM_SUCCESS) {
        bvmm_surface_engine_shutdown();
        bvmm_pools_shutdown();
        bvmm_heap_subsystem_shutdown();
        bvmm_handle_table_shutdown();
        return res;
    }

    /* 7. Initialize Reference, Residency & Lifetime Engine (BRRLE V1.0) */
    res = brrle_init();
    if (res != BVMM_SUCCESS) {
        btfe_shutdown();
        bvmm_surface_engine_shutdown();
        bvmm_pools_shutdown();
        bvmm_heap_subsystem_shutdown();
        bvmm_handle_table_shutdown();
        return res;
    }

    /* 8. Initialize Production Fence & Synchronization Engine (BFSE V1.0) */
    res = bfse_init();
    if (res != BVMM_SUCCESS) {
        brrle_shutdown();
        btfe_shutdown();
        bvmm_surface_engine_shutdown();
        bvmm_pools_shutdown();
        bvmm_heap_subsystem_shutdown();
        bvmm_handle_table_shutdown();
        return res;
    }

    /* 9. Initialize Production Memory Policy Engine (BMPRE V1.0) */
    res = bmpre_init();
    if (res != BVMM_SUCCESS) {
        bfse_shutdown();
        brrle_shutdown();
        btfe_shutdown();
        bvmm_surface_engine_shutdown();
        bvmm_pools_shutdown();
        bvmm_heap_subsystem_shutdown();
        bvmm_handle_table_shutdown();
        return res;
    }

    /* 10. Initialize Production Live Migration & Defrag Engine (BLMDE V1.0) */
    res = blmde_init();
    if (res != BVMM_SUCCESS) {
        bmpre_shutdown();
        bfse_shutdown();
        brrle_shutdown();
        btfe_shutdown();
        bvmm_surface_engine_shutdown();
        bvmm_pools_shutdown();
        bvmm_heap_subsystem_shutdown();
        bvmm_handle_table_shutdown();
        return res;
    }

    /* 11. Initialize Production GPU Hardware Abstraction Layer (BGHAL V1.0) */
    res = bghal_init();
    if (res != BVMM_SUCCESS) {
        blmde_shutdown();
        bmpre_shutdown();
        bfse_shutdown();
        brrle_shutdown();
        btfe_shutdown();
        bvmm_surface_engine_shutdown();
        bvmm_pools_shutdown();
        bvmm_heap_subsystem_shutdown();
        bvmm_handle_table_shutdown();
        return res;
    }

    /* 12. Initialize Production Cross-Process GPU Resource Sharing Engine (BCPSE V1.0) */
    res = bcpse_init();
    if (res != BVMM_SUCCESS) {
        bghal_shutdown();
        blmde_shutdown();
        bmpre_shutdown();
        bfse_shutdown();
        brrle_shutdown();
        btfe_shutdown();
        bvmm_surface_engine_shutdown();
        bvmm_pools_shutdown();
        bvmm_heap_subsystem_shutdown();
        bvmm_handle_table_shutdown();
        return res;
    }

    /* 13. Initialize Production Optimization & Freeze Engine (BPOE V1.0) */
    res = bpoe_init();
    if (res != BVMM_SUCCESS) {
        bcpse_shutdown();
        bghal_shutdown();
        blmde_shutdown();
        bmpre_shutdown();
        bfse_shutdown();
        brrle_shutdown();
        btfe_shutdown();
        bvmm_surface_engine_shutdown();
        bvmm_pools_shutdown();
        bvmm_heap_subsystem_shutdown();
        bvmm_handle_table_shutdown();
        return res;
    }

    g_bvmm_initialized = true;
    display_print("[BVMM] BOS VRAM Memory Manager Phase 12 Production Optimization Active (V1.0 PRODUCTION)\n");
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_shutdown(void) {
    if (!g_bvmm_initialized) return BVMM_ERR_NOT_INITIALIZED;

    bpoe_shutdown();
    bcpse_shutdown();
    bghal_shutdown();
    blmde_shutdown();
    bmpre_shutdown();
    bfse_shutdown();
    brrle_shutdown();
    btfe_shutdown();
    bvmm_surface_engine_shutdown();
    bvmm_pools_shutdown();
    bvmm_heap_subsystem_shutdown();
    bvmm_handle_table_shutdown();

    g_bvmm_initialized = false;
    display_print("[BVMM] BOS VRAM Memory Manager Phase 12 Shutdown Cleanly\n");
    return BVMM_SUCCESS;
}

bool bvmm_is_initialized(void) {
    return g_bvmm_initialized;
}

bvmm_result_t bvmm_get_stats(bvmm_stats_t* out_stats) {
    if (!out_stats) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_bvmm_initialized) return BVMM_ERR_NOT_INITIALIZED;

    bvmm_heap_t* primary_vram = bvmm_heap_get_primary(BVMM_DOMAIN_VRAM, BVMM_POOL_LARGE);
    bvmm_heap_t* primary_gtt  = bvmm_heap_get_primary(BVMM_DOMAIN_GTT, BVMM_POOL_LARGE);

    out_stats->total_vram_bytes = primary_vram ? primary_vram->total_size_bytes : 0;
    out_stats->used_vram_bytes  = primary_vram ? primary_vram->used_size_bytes : 0;
    out_stats->free_vram_bytes  = primary_vram ? primary_vram->free_size_bytes : 0;
    out_stats->largest_free_vram_block = primary_vram ? primary_vram->largest_free_block_bytes : 0;

    out_stats->total_gtt_bytes  = primary_gtt ? primary_gtt->total_size_bytes : 0;
    out_stats->used_gtt_bytes   = primary_gtt ? primary_gtt->used_size_bytes : 0;
    out_stats->free_gtt_bytes   = primary_gtt ? primary_gtt->free_size_bytes : 0;

    return BVMM_SUCCESS;
}
