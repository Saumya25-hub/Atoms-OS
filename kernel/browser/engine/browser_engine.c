#include "browser_engine.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

static ATRIX_BrowserEngineMetrics g_engine_metrics;

void ATRIX_BrowserEngine_Init(void) {
    g_engine_metrics.active_tabs_count = 1;
    g_engine_metrics.dom_nodes_count = 0;
    g_engine_metrics.memory_used_bytes = 128 * 1024;
    g_engine_metrics.engine_initialized = true;

    bwe_log("INFO", "ATRIX Native Browser Engine v1.0 Initialized");
}

void ATRIX_BrowserEngine_GetMetrics(ATRIX_BrowserEngineMetrics* out_metrics) {
    if (out_metrics) {
        *out_metrics = g_engine_metrics;
    }
}
