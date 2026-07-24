#ifndef ATRIX_BROWSER_ENGINE_H
#define ATRIX_BROWSER_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX Browser Engine v1.0 Core Engine (Phase 12)
// ============================================================

#define ATRIX_ENGINE_VERSION_MAJOR 1
#define ATRIX_ENGINE_VERSION_MINOR 0
#define ATRIX_MAX_TABS             16

typedef struct {
    uint32_t active_tabs_count;
    uint32_t dom_nodes_count;
    uint32_t memory_used_bytes;
    bool     engine_initialized;
} ATRIX_BrowserEngineMetrics;

void ATRIX_BrowserEngine_Init(void);
void ATRIX_BrowserEngine_GetMetrics(ATRIX_BrowserEngineMetrics* out_metrics);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_BROWSER_ENGINE_H
