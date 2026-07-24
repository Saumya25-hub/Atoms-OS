#ifndef ATRIX_LAYOUT_ENGINE_H
#define ATRIX_LAYOUT_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX Layout Flow Engine Subsystem
// ============================================================

typedef struct {
    int32_t viewport_width;
    int32_t viewport_height;
    int32_t content_height;
} ATRIX_LayoutMetrics;

void ATRIX_LayoutEngine_Init(void);
void ATRIX_LayoutEngine_ComputeLayout(void* dom_tree, int32_t vw, int32_t vh);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_LAYOUT_ENGINE_H
