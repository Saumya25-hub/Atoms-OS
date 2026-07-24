#include "paint_engine.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

static ATRIX_PaintMetrics g_paint_metrics;

void ATRIX_PaintEngine_Init(void) {
    g_paint_metrics.total_paints = 0;
    g_paint_metrics.pixels_drawn = 0;

    bwe_log("INFO", "ATRIX Software Paint Engine Subsystem Initialized");
}

void ATRIX_PaintEngine_PaintTree(const void* render_tree, void* fb_target, int32_t scroll_y) {
    (void)render_tree;
    (void)fb_target;
    (void)scroll_y;

    g_paint_metrics.total_paints++;
    g_paint_metrics.pixels_drawn += 800 * 600;
}
