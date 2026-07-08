#ifndef KERNEL_POINTER_BOUNDS_H
#define KERNEL_POINTER_BOUNDS_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 3: Multi-Monitor Display Bounds Registry
// ============================================================================

#define POINTER_MAX_DISPLAYS 4

typedef struct {
    int32_t  origin_x;
    int32_t  origin_y;
    uint32_t width;
    uint32_t height;
    uint32_t dpi_scale; // 100 = 100% scale (default)
    bool     active;
} PointerDisplayBounds;

// Initialize bounds registry with primary display dimensions
void pointer_bounds_init(uint32_t primary_width, uint32_t primary_height);

// Register a secondary monitor or virtual desktop bounding rectangle
bool pointer_bounds_add_display(int32_t origin_x, int32_t origin_y, uint32_t width, uint32_t height, uint32_t dpi_scale);

// Authoritatively clamp pointer coordinates against active display rectangles
void pointer_bounds_clamp(int32_t* inout_x, int32_t* inout_y);

#endif // KERNEL_POINTER_BOUNDS_H
