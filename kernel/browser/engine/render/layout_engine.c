#include "layout_engine.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

static ATRIX_LayoutMetrics g_layout_metrics;

void ATRIX_LayoutEngine_Init(void) {
    g_layout_metrics.viewport_width = 800;
    g_layout_metrics.viewport_height = 600;
    g_layout_metrics.content_height = 1200;

    bwe_log("INFO", "ATRIX Layout Flow Engine Subsystem Initialized");
}

void ATRIX_LayoutEngine_ComputeLayout(void* dom_tree, int32_t vw, int32_t vh) {
    (void)dom_tree;
    g_layout_metrics.viewport_width = vw;
    g_layout_metrics.viewport_height = vh;
}
