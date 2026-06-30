#ifndef KERNEL_BOCOMPOSITOR_CLIP_H
#define KERNEL_BOCOMPOSITOR_CLIP_H

#include "compositor_types.h"

// Initialize clipping engine with global screen boundaries
void BOCompositorClip_Init(int32_t screen_width, int32_t screen_height);

// Push a rectangular bounding box onto the clipping stack (intersecting with current clip)
void BOCompositorClip_PushRect(const BOCompositorRect* rect);

// Pop the top clipping rectangle from stack
void BOCompositorClip_PopRect(void);

// Get current effective intersection clip rectangle
bool BOCompositorClip_GetCurrent(BOCompositorRect* out_rect);

// Compute exact rectangular intersection of a and b. Returns true if valid non-empty intersection.
bool BOCompositorClip_Intersect(const BOCompositorRect* a, const BOCompositorRect* b, BOCompositorRect* out_res);

#endif // KERNEL_BOCOMPOSITOR_CLIP_H
