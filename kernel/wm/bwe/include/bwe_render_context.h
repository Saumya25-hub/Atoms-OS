#ifndef BWE_RENDER_CONTEXT_H
#define BWE_RENDER_CONTEXT_H

#include "bwe.h"
#include "kernel/graphics/BSPE/include/bspe.h"

// Returns true if the coordinate is within the current active clipping rectangle.
// If no clip rectangle is active, returns true if it's within the screen bounds.
bool BWE_RenderContext_CheckClip(int32_t x, int32_t y);

// Wrapped PutPixel that respects BWE compositor clipping
void BWE_RenderContext_PutPixel(const BVFramebuffer* fb, int32_t x, int32_t y, uint32_t color);

#endif // BWE_RENDER_CONTEXT_H
