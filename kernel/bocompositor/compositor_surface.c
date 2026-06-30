#include "compositor_surface.h"

static BOCompositorSurface g_surface_pool[BOCOMPOSITOR_MAX_SURFACES];
static uint32_t g_active_surface_count = 0;

void BOCompositorSurface_InitPool(void) {
    g_active_surface_count = 0;
    for (uint32_t i = 0; i < BOCOMPOSITOR_MAX_SURFACES; i++) {
        g_surface_pool[i].id = 0;
        g_surface_pool[i].active = false;
        g_surface_pool[i].x = 0;
        g_surface_pool[i].y = 0;
        g_surface_pool[i].width = 0;
        g_surface_pool[i].height = 0;
        g_surface_pool[i].z_order = 0;
        g_surface_pool[i].state = BOCOMPOSITOR_STATE_HIDDEN;
        g_surface_pool[i].flags = 0;
        g_surface_pool[i].is_dirty = false;
        g_surface_pool[i].surface_handle = NULL;
    }
}

BOCompositorSurface* BOCompositorSurface_Alloc(uint32_t id, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t z_order, void* surface_handle) {
    // If already exists, return existing
    BOCompositorSurface* existing = BOCompositorSurface_Find(id);
    if (existing) {
        existing->x = x;
        existing->y = y;
        existing->width = width;
        existing->height = height;
        existing->z_order = z_order;
        existing->surface_handle = surface_handle;
        return existing;
    }

    for (uint32_t i = 0; i < BOCOMPOSITOR_MAX_SURFACES; i++) {
        if (!g_surface_pool[i].active) {
            g_surface_pool[i].id = id;
            g_surface_pool[i].active = true;
            g_surface_pool[i].x = x;
            g_surface_pool[i].y = y;
            g_surface_pool[i].width = width;
            g_surface_pool[i].height = height;
            g_surface_pool[i].z_order = z_order;
            g_surface_pool[i].state = BOCOMPOSITOR_STATE_VISIBLE;
            g_surface_pool[i].flags = BOCOMPOSITOR_FLAG_VISIBLE;
            g_surface_pool[i].is_dirty = true;
            g_surface_pool[i].clip_rect = (BOCompositorRect){x, y, width, height};
            g_surface_pool[i].invalid_rect = (BOCompositorRect){x, y, width, height};
            g_surface_pool[i].surface_handle = surface_handle;
            g_active_surface_count++;
            return &g_surface_pool[i];
        }
    }
    return NULL; // Pool full
}

void BOCompositorSurface_Free(uint32_t id) {
    for (uint32_t i = 0; i < BOCOMPOSITOR_MAX_SURFACES; i++) {
        if (g_surface_pool[i].active && g_surface_pool[i].id == id) {
            g_surface_pool[i].active = false;
            g_surface_pool[i].id = 0;
            g_surface_pool[i].surface_handle = NULL;
            if (g_active_surface_count > 0) {
                g_active_surface_count--;
            }
            return;
        }
    }
}

BOCompositorSurface* BOCompositorSurface_Find(uint32_t id) {
    for (uint32_t i = 0; i < BOCOMPOSITOR_MAX_SURFACES; i++) {
        if (g_surface_pool[i].active && g_surface_pool[i].id == id) {
            return &g_surface_pool[i];
        }
    }
    return NULL;
}

BOCompositorSurface* BOCompositorSurface_GetByIndex(uint32_t index) {
    if (index >= BOCOMPOSITOR_MAX_SURFACES) return NULL;
    return &g_surface_pool[index];
}

uint32_t BOCompositorSurface_GetActiveCount(void) {
    return g_active_surface_count;
}

void BOCompositorSurface_SetBounds(uint32_t id, int32_t x, int32_t y, int32_t width, int32_t height) {
    BOCompositorSurface* surf = BOCompositorSurface_Find(id);
    if (surf) {
        surf->x = x;
        surf->y = y;
        surf->width = width;
        surf->height = height;
        surf->is_dirty = true;
    }
}

void BOCompositorSurface_SetVisible(uint32_t id, bool visible) {
    BOCompositorSurface* surf = BOCompositorSurface_Find(id);
    if (surf) {
        if (visible) {
            surf->state = BOCOMPOSITOR_STATE_VISIBLE;
            surf->flags |= BOCOMPOSITOR_FLAG_VISIBLE;
        } else {
            surf->state = BOCOMPOSITOR_STATE_HIDDEN;
            surf->flags &= ~BOCOMPOSITOR_FLAG_VISIBLE;
        }
        surf->is_dirty = true;
    }
}
