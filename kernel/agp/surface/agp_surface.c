// Engine 3: Surface Engine
#include "../include/agp_api.h"
#include "kernel/core/lib/include/string.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

static AGPSurface s_surface_pool[AGP_MAX_SURFACES];
static bool s_surface_used[AGP_MAX_SURFACES];

AGPSurface* AGP_CreateSurface(uint32_t w, uint32_t h, AGPFormat fmt) {
    if (w == 0 || h == 0) return NULL;

    for (uint32_t i = 0; i < AGP_MAX_SURFACES; i++) {
        if (!s_surface_used[i]) {
            s_surface_used[i] = true;
            AGPSurface* surf = &s_surface_pool[i];
            surf->width = w;
            surf->height = h;
            surf->format = fmt;
            surf->is_onscreen = false;
            surf->window_id = 0;

            size_t bytes = (size_t)w * h * sizeof(uint32_t);
            surf->pixels = (uint32_t*)kmalloc(bytes);
            if (!surf->pixels) {
                s_surface_used[i] = false;
                return NULL;
            }
            memset(surf->pixels, 0, bytes);
            return surf;
        }
    }
    return NULL;
}

void AGP_DestroySurface(AGPSurface* surface) {
    if (!surface) return;
    for (uint32_t i = 0; i < AGP_MAX_SURFACES; i++) {
        if (s_surface_used[i] && &s_surface_pool[i] == surface) {
            if (surface->pixels) {
                kfree(surface->pixels);
                surface->pixels = NULL;
            }
            s_surface_used[i] = false;
            return;
        }
    }
}
