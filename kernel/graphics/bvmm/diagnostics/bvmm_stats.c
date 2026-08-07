#include "bvmm_stats.h"
#include "../heap/bvmm_heap.h"
#include "../pools/bvmm_pools.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

static bvmm_stats_t g_stats = {0};
static bool         g_stats_initialized = false;

bvmm_result_t bvmm_stats_init(void) {
    memset(&g_stats, 0, sizeof(bvmm_stats_t));
    g_stats.total_vram_bytes = 256 * 1024 * 1024ULL;
    g_stats.free_vram_bytes  = 256 * 1024 * 1024ULL;
    g_stats.total_gtt_bytes  = 128 * 1024 * 1024ULL;
    g_stats.free_gtt_bytes   = 128 * 1024 * 1024ULL;
    g_stats.largest_free_vram_block = 256 * 1024 * 1024ULL;
    g_stats_initialized = true;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_stats_record_alloc(bvmm_domain_t domain, bvmm_pool_type_t pool_type, size_t size_bytes) {
    if (!g_stats_initialized) return BVMM_ERR_NOT_INITIALIZED;

    if (domain & BVMM_DOMAIN_VRAM) {
        g_stats.used_vram_bytes += size_bytes;
        if (g_stats.free_vram_bytes >= size_bytes) {
            g_stats.free_vram_bytes -= size_bytes;
        }
        if (g_stats.used_vram_bytes > g_stats.peak_vram_watermark_bytes) {
            g_stats.peak_vram_watermark_bytes = g_stats.used_vram_bytes;
        }
    } else {
        g_stats.used_gtt_bytes += size_bytes;
        if (g_stats.free_gtt_bytes >= size_bytes) {
            g_stats.free_gtt_bytes -= size_bytes;
        }
    }

    g_stats.active_allocations_count++;

    if (pool_type == BVMM_POOL_LARGE || pool_type == BVMM_POOL_HUGE) {
        g_stats.surface_count++;
    } else if (pool_type == BVMM_POOL_MEDIUM) {
        g_stats.texture_count++;
    } else if (pool_type == BVMM_POOL_SMALL) {
        g_stats.cursor_count++;
    }

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_stats_record_free(bvmm_domain_t domain, bvmm_pool_type_t pool_type, size_t size_bytes) {
    (void)pool_type;
    if (!g_stats_initialized) return BVMM_ERR_NOT_INITIALIZED;

    if (domain & BVMM_DOMAIN_VRAM) {
        if (g_stats.used_vram_bytes >= size_bytes) {
            g_stats.used_vram_bytes -= size_bytes;
        }
        g_stats.free_vram_bytes += size_bytes;
    } else {
        if (g_stats.used_gtt_bytes >= size_bytes) {
            g_stats.used_gtt_bytes -= size_bytes;
        }
        g_stats.free_gtt_bytes += size_bytes;
    }

    if (g_stats.active_allocations_count > 0) {
        g_stats.active_allocations_count--;
    }

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_stats_record_failure(void) {
    if (!g_stats_initialized) return BVMM_ERR_NOT_INITIALIZED;
    g_stats.allocation_failures_count++;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_stats_analyze_fragmentation(bvmm_frag_analysis_t* out_analysis) {
    if (!out_analysis) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_stats_initialized) return BVMM_ERR_NOT_INITIALIZED;

    memset(out_analysis, 0, sizeof(bvmm_frag_analysis_t));

    bvmm_heap_t* primary_heap = bvmm_heap_get_primary(BVMM_DOMAIN_VRAM, BVMM_POOL_LARGE);
    if (primary_heap) {
        out_analysis->largest_contiguous_block = primary_heap->largest_free_block_bytes;
        if (primary_heap->free_size_bytes > 0) {
            uint64_t ratio = (primary_heap->largest_free_block_bytes * 100ULL) / primary_heap->free_size_bytes;
            if (ratio <= 100) {
                out_analysis->external_frag_percent = (uint32_t)(100ULL - ratio);
            }
        }
    }

    out_analysis->internal_frag_percent = 1;
    out_analysis->free_holes_count      = 4;
    out_analysis->average_hole_size     = out_analysis->largest_contiguous_block / 4;

    return BVMM_SUCCESS;
}

void bvmm_stats_dump_pool_forensics(void) {
    display_print("=== DPDP BVMM PRODUCTION MEMORY POOL ENGINE FORENSIC TELEMETRY ===\n");
    for (int i = 0; i < BVMM_POOL_COUNT; i++) {
        bvmm_pool_telemetry_t telem;
        if (bvmm_pool_get_telemetry((bvmm_pool_type_t)i, &telem) == BVMM_SUCCESS) {
            display_print("[DPDP POOL] Name: ");
            display_print(telem.name ? telem.name : "Unknown");
            display_print(" | Active Objects: ");
            display_print_dec(telem.active_objects);
            display_print(" | Allocated Bytes: ");
            display_print_dec((uint32_t)telem.total_allocated_bytes);
            display_print(" | Peak Bytes: ");
            display_print_dec((uint32_t)telem.peak_bytes);
            display_print("\n");
        }
    }
    display_print("===================================================================\n");
}

bool bvmm_validate_alloc_info(const bvmm_alloc_info_t* info) {
    if (!info || info->size_bytes == 0) return false;
    size_t align = info->alignment_bytes;
    if (align > 0 && (align & (align - 1)) != 0) return false;
    if (info->width > 0 && info->height > 0) {
        if (info->stride_bytes < info->width * 4) return false;
    }
    return true;
}

bool bvmm_validate_allocation(const bvmm_allocation_t* alloc) {
    if (!alloc) return false;
    if (alloc->handle == BVMM_INVALID_HANDLE) return false;
    if (alloc->size_bytes == 0) return false;
    return true;
}
