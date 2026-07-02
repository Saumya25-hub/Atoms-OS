#ifndef KERNEL_BOCOMPOSITOR_DAMAGE_H
#define KERNEL_BOCOMPOSITOR_DAMAGE_H

#include "compositor_types.h"

// Initialize dirty rectangle damage tracking
void BOCompositorDamage_Init(void);

// Add an invalid region to the damage tracker
void BOCompositorDamage_AddRect(int32_t x, int32_t y, int32_t width, int32_t height);

// Clear all damage rectangles after frame composition and page swap
void BOCompositorDamage_Clear(void);

// Check if any damage exists for current frame cycle
bool BOCompositorDamage_HasDamage(void);

// Get total count of dirty rectangles
uint32_t BOCompositorDamage_GetCount(void);

// Get specific dirty rectangle by index
const BOCompositorRect* BOCompositorDamage_GetRect(uint32_t index);

#endif // KERNEL_BOCOMPOSITOR_DAMAGE_H
