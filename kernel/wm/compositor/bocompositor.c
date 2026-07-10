#include "bocompositor.h"

static BOCompositorStats g_stats;
static bool g_debug_overlay_enabled = false;
static BOCompositorRenderCallback g_render_callback = NULL;
uint32_t g_compositor_current_surface_id = 0;

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

void BOCompositor_Initialize(void) {
    BOCompositorSurface_InitPool();
    BOCompositorStack_Init();
    BOCompositorClip_Init((int32_t)g_kernel_screen_width, (int32_t)g_kernel_screen_height);
    BOCompositorDamage_Init();

    g_stats.surface_count = 0;
    g_stats.visible_windows = 0;
    g_stats.hidden_windows = 0;
    g_stats.dirty_rectangles = 0;
    g_stats.clip_count = 0;
    g_stats.occlusion_count = 0;
    g_stats.batch_count = 0;
    g_stats.compose_time_ms = 0;
}

void BOCompositor_Shutdown(void) {
    BOCompositorSurface_InitPool();
    BOCompositorStack_Init();
    BOCompositorDamage_Clear();
}

void BOCompositor_SetRenderCallback(BOCompositorRenderCallback callback) {
    g_render_callback = callback;
}

bool BOCompositor_RegisterSurface(uint32_t id, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t z_order, void* surface_handle) {
    BOCompositorSurface* surf = BOCompositorSurface_Alloc(id, x, y, width, height, z_order, surface_handle);
    if (!surf) return false;

    BOCompositorStack_Insert(id, z_order);
    BOCompositorDamage_AddRect(x, y, width, height);
    return true;
}

bool BOCompositor_RemoveSurface(uint32_t id) {
    BOCompositorSurface* surf = BOCompositorSurface_Find(id);
    if (surf) {
        BOCompositorDamage_AddRect(surf->x, surf->y, surf->width, surf->height);
    }
    BOCompositorStack_Remove(id);
    BOCompositorSurface_Free(id);
    return true;
}

void BOCompositor_Invalidate(int32_t x, int32_t y, int32_t width, int32_t height) {
    BOCompositorDamage_AddRect(x, y, width, height);
}

void BOCompositor_BeginFrame(void) {
    // Reset transient frame counters
    g_stats.clip_count = 0;
    g_stats.occlusion_count = 0;
    g_stats.batch_count = 0;
}

void BOCompositor_ClearDamage(void) {
    BOCompositorDamage_Clear();
}

void BOCompositor_AddDamage(int32_t x, int32_t y, int32_t width, int32_t height) {
    BOCompositorDamage_AddRect(x, y, width, height);
}

bool BOCompositor_HasDamage(void) {
    return BOCompositorDamage_HasDamage();
}

uint32_t BOCompositor_GetDamageCount(void) {
    return BOCompositorDamage_GetCount();
}

bool BOCompositor_BringToFront(uint32_t id) {
    BOCompositorStack_BringToFront(id);
    BOCompositorSurface* surf = BOCompositorSurface_Find(id);
    if (surf) {
        BOCompositorDamage_AddRect(surf->x, surf->y, surf->width, surf->height);
        return true;
    }
    return false;
}

bool BOCompositor_SendToBack(uint32_t id) {
    BOCompositorStack_SendToBack(id);
    BOCompositorSurface* surf = BOCompositorSurface_Find(id);
    if (surf) {
        BOCompositorDamage_AddRect(surf->x, surf->y, surf->width, surf->height);
        return true;
    }
    return false;
}

bool BOCompositor_SetFocus(uint32_t id) {
    BOCompositorStack_SetFocus(id);
    return true;
}

void BOCompositor_Update(uint32_t id, int32_t x, int32_t y, int32_t width, int32_t height, bool visible) {
    BOCompositorSurface* surf = BOCompositorSurface_Find(id);
    if (surf) {
        // Add damage for old bounds
        BOCompositorDamage_AddRect(surf->x, surf->y, surf->width, surf->height);
        
        BOCompositorSurface_SetBounds(id, x, y, width, height);
        BOCompositorSurface_SetVisible(id, visible);
        
        // Add damage for new bounds if visible
        if (visible) {
            BOCompositorDamage_AddRect(x, y, width, height);
        }
    }
}

void BOCompositor_SetDebugOverlay(bool enabled) {
    g_debug_overlay_enabled = enabled;
}

bool BOCompositor_IsDebugOverlayEnabled(void) {
    return g_debug_overlay_enabled;
}

const BOCompositorStats* BOCompositor_GetStats(void) {
    g_stats.surface_count = BOCompositorSurface_GetActiveCount();
    g_stats.dirty_rectangles = BOCompositorDamage_GetCount();
    return &g_stats;
}

// Composition loop (Zero Allocations!)
void BOCompositor_ComposeFrame(void) {
    uint32_t sorted_ids[BOCOMPOSITOR_MAX_SURFACES];
    uint32_t visible_count = BOCompositorStack_GetSortedVisible(sorted_ids, BOCOMPOSITOR_MAX_SURFACES);

    g_stats.visible_windows = visible_count;
    g_stats.hidden_windows = BOCompositorSurface_GetActiveCount() - visible_count;

    bool occluded[BOCOMPOSITOR_MAX_SURFACES];
    for (uint32_t i = 0; i < visible_count; i++) {
        occluded[i] = false;
    }

    // Pass 1: Occlusion evaluation (Top-to-bottom Z scan)
    for (int32_t i = (int32_t)visible_count - 1; i >= 0; i--) {
        BOCompositorSurface* surf = BOCompositorSurface_Find(sorted_ids[i]);
        if (!surf) continue;

        // Check if any surface j above i completely covers i
        for (int32_t j = i + 1; j < (int32_t)visible_count; j++) {
            BOCompositorSurface* above = BOCompositorSurface_Find(sorted_ids[j]);
            if (!above) continue;

            // Only consider opaque surfaces for complete occlusion
            if (above->flags & BOCOMPOSITOR_FLAG_OPAQUE) {
                if (above->x <= surf->x &&
                    above->y <= surf->y &&
                    above->x + above->width >= surf->x + surf->width &&
                    above->y + above->height >= surf->y + surf->height) {
                    occluded[i] = true;
                    g_stats.occlusion_count++;
                    break;
                }
            }
        }
    }

    // Pass 2: Back-to-Front Render Execution
    for (uint32_t i = 0; i < visible_count; i++) {
        if (occluded[i]) continue; // Skip completely occluded surface!

        BOCompositorSurface* surf = BOCompositorSurface_Find(sorted_ids[i]);
        if (!surf) continue;

        BOCompositorRect surf_rect = {surf->x, surf->y, surf->width, surf->height};
        BOCompositorClip_PushRect(&surf_rect);
        g_stats.clip_count++;

        BOCompositorRect effective_clip;
        if (BOCompositorClip_GetCurrent(&effective_clip)) {
            if (g_render_callback) {
                g_render_callback(surf->surface_handle, &effective_clip);
                g_stats.batch_count++;
            }
        }

        BOCompositorClip_PopRect();
    }
}
