#ifndef KERNEL_BOCOMPOSITOR_STACK_H
#define KERNEL_BOCOMPOSITOR_STACK_H

#include "compositor_types.h"

// Initialize stack tracking
void BOCompositorStack_Init(void);

// Insert or update a surface in the Z-order stack
void BOCompositorStack_Insert(uint32_t id, uint32_t z_order);

// Remove a surface from the stack
void BOCompositorStack_Remove(uint32_t id);

// Bring a surface to the top of its layer
void BOCompositorStack_BringToFront(uint32_t id);

// Send a surface to the bottom of its layer
void BOCompositorStack_SendToBack(uint32_t id);

// Mark a surface as the currently focused window
void BOCompositorStack_SetFocus(uint32_t id);

// Get currently focused surface ID
uint32_t BOCompositorStack_GetFocusedID(void);

// Collect all visible surfaces sorted in deterministic back-to-front Z-order
// Returns number of IDs written to out_ids
uint32_t BOCompositorStack_GetSortedVisible(uint32_t* out_ids, uint32_t max_count);

#endif // KERNEL_BOCOMPOSITOR_STACK_H
