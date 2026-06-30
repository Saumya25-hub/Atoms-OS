#ifndef KERNEL_BOCOMPOSITOR_BOCOMPOSITOR_H
#define KERNEL_BOCOMPOSITOR_BOCOMPOSITOR_H

#include "compositor_types.h"
#include "compositor_surface.h"
#include "compositor_stack.h"
#include "compositor_clip.h"
#include "compositor_damage.h"

// ============================================================
// BOCOMPOSITOR ENGINE v2 Public Master API
// ============================================================

typedef void (*BOCompositorRenderCallback)(void* surface_handle, const BOCompositorRect* clip_rect);

// Initialize the desktop composition engine and all static subsystem pools
void BOCompositor_Initialize(void);

// Shutdown the engine
void BOCompositor_Shutdown(void);

// Set external render hook (invoked per visible unoccluded surface during frame composition)
void BOCompositor_SetRenderCallback(BOCompositorRenderCallback callback);

// Register a surface / window into the compositor
bool BOCompositor_RegisterSurface(uint32_t id, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t z_order, void* surface_handle);

// Remove a surface from the compositor
bool BOCompositor_RemoveSurface(uint32_t id);

// Invalidate a specific screen rectangle, marking it dirty for the next frame cycle
void BOCompositor_Invalidate(int32_t x, int32_t y, int32_t width, int32_t height);

// Execute deterministic frame composition (Z-Sort -> Clip -> Occlusion -> Render Callbacks)
void BOCompositor_ComposeFrame(void);

// Bring surface to front of Z-order
bool BOCompositor_BringToFront(uint32_t id);

// Send surface to back of Z-order
bool BOCompositor_SendToBack(uint32_t id);

// Set window focus
bool BOCompositor_SetFocus(uint32_t id);

// Update surface geometry or visibility
void BOCompositor_Update(uint32_t id, int32_t x, int32_t y, int32_t width, int32_t height, bool visible);

// Frame cycle hooks (for BOHEART integration)
void BOCompositor_BeginFrame(void);
void BOCompositor_ClearDamage(void);

// Backwards-compatibility aliases for Phase 1 damage API
void BOCompositor_AddDamage(int32_t x, int32_t y, int32_t width, int32_t height);
bool BOCompositor_HasDamage(void);
uint32_t BOCompositor_GetDamageCount(void);

// Debug overlay APIs
void BOCompositor_SetDebugOverlay(bool enabled);
bool BOCompositor_IsDebugOverlayEnabled(void);
const BOCompositorStats* BOCompositor_GetStats(void);

#endif // KERNEL_BOCOMPOSITOR_BOCOMPOSITOR_H
