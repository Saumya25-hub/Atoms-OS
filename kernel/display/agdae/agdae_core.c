/**
 * @file agdae_core.c
 * @brief ATOMS OS Display Adaptation Engine - Core State Manager
 */

#include "agdae.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/lib/include/crash_log.h"

AGDAE_Metrics g_agdae_metrics;

void AGDAE_Initialize(uint32_t hw_physical_w, uint32_t hw_physical_h, uint32_t optimal_logical_w, uint32_t optimal_logical_h) {
    memset(&g_agdae_metrics, 0, sizeof(AGDAE_Metrics));

    if (hw_physical_w == 0 || hw_physical_h == 0) {
        hw_physical_w = 1024;
        hw_physical_h = 768;
    }
    if (optimal_logical_w == 0 || optimal_logical_h == 0) {
        optimal_logical_w = hw_physical_w;
        optimal_logical_h = hw_physical_h;
    }

    g_agdae_metrics.physical_width = hw_physical_w;
    g_agdae_metrics.physical_height = hw_physical_h;
    
    g_agdae_metrics.logical_width = optimal_logical_w;
    g_agdae_metrics.logical_height = optimal_logical_h;

    /* Determine scaling factor based on logical vs physical relationship or capability */
    if (optimal_logical_w < hw_physical_w || optimal_logical_h < hw_physical_h) {
        g_agdae_metrics.scale_factor_pct = 100;
        g_agdae_metrics.logical_width = optimal_logical_w;
        g_agdae_metrics.logical_height = optimal_logical_h;
        g_agdae_metrics.is_scaled = false;
        g_agdae_metrics.is_letterboxed = true;
    } else {
        g_agdae_metrics.scale_factor_pct = 100;
        g_agdae_metrics.logical_width = hw_physical_w;
        g_agdae_metrics.logical_height = hw_physical_h;
        g_agdae_metrics.is_scaled = false;
        g_agdae_metrics.is_letterboxed = false;
    }
    
    if (g_agdae_metrics.scale_factor_pct >= 150) {
        g_agdae_metrics.is_hidpi = true;
    }

    /* Compute initial geometry */
    AGDAE_Geometry_Update(&g_agdae_metrics);

    crash_log_add("[AGDAE] Engine Initialized");
    AGDAE_DumpDiagnostics();
}

const AGDAE_Metrics* AGDAE_GetMetrics(void) {
    return &g_agdae_metrics;
}
