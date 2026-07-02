#ifndef KERNEL_BOCOMPOSITOR_SURFACE_H
#define KERNEL_BOCOMPOSITOR_SURFACE_H

#include "compositor_types.h"

// Initialize surface pool memory
void BOCompositorSurface_InitPool(void);

// Allocate a new compositor surface entry in the static pool
BOCompositorSurface* BOCompositorSurface_Alloc(uint32_t id, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t z_order, void* surface_handle);

// Release a surface slot
void BOCompositorSurface_Free(uint32_t id);

// Find surface entry by ID
BOCompositorSurface* BOCompositorSurface_Find(uint32_t id);

// Retrieve surface slot by raw pool index (0 to BOCOMPOSITOR_MAX_SURFACES-1)
BOCompositorSurface* BOCompositorSurface_GetByIndex(uint32_t index);

// Get total active surfaces in pool
uint32_t BOCompositorSurface_GetActiveCount(void);

// Update geometry
void BOCompositorSurface_SetBounds(uint32_t id, int32_t x, int32_t y, int32_t width, int32_t height);

// Update visibility
void BOCompositorSurface_SetVisible(uint32_t id, bool visible);

#endif // KERNEL_BOCOMPOSITOR_SURFACE_H
