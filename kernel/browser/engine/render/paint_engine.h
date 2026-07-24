#ifndef ATRIX_PAINT_ENGINE_H
#define ATRIX_PAINT_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX Hardware Accelerated Software Paint Engine Subsystem
// ============================================================

typedef struct {
    uint32_t total_paints;
    uint32_t pixels_drawn;
} ATRIX_PaintMetrics;

void ATRIX_PaintEngine_Init(void);
void ATRIX_PaintEngine_PaintTree(const void* render_tree, void* fb_target, int32_t scroll_y);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_PAINT_ENGINE_H
