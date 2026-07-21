#include "wallpaper_manager.h"
#include "kernel/display/agdae/agdae.h"
#include "kernel/media/bopawn/bopawn.h"
#include "kernel/media/bopawn/formats/image_format.h"
#include "kernel/gui/surface/surface.h"
#include "kernel/shell/desktop_shell/desktop_shell.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/gui/animation/animation_fade.h"
#include <stddef.h>

// BMLE Ownership Phase 1: Include cache to remove raw allocations
extern void image_cache_remove(const char* path);

static WallpaperEntry* g_current_wallpaper = NULL;
static WallpaperScaleMode g_current_scale_mode = WALLPAPER_SCALE_STRETCH;

void wallpaper_manager_init(void) {
    wallpaper_registry_init();
    
    // Load default
    wallpaper_set(1);
}

static bool _apply_wallpaper(struct BOSSurface* raw_surf) {
    if (!raw_surf) return false;
    
    
    
    
    struct BOSSurface* scaled = wallpaper_scaler_scale(raw_surf, (uint32_t)AGDAE_GetMetrics()->desktop_rect.width, (uint32_t)AGDAE_GetMetrics()->desktop_rect.height, g_current_scale_mode, 0xFF0B1120);
    
    if (scaled) {
        wallpaper_transition(scaled, 250);
        return true;
    }
    return false;
}

bool wallpaper_load(const char* path) {
    return wallpaper_set_path(path);
}

bool wallpaper_set(uint32_t id) {
    WallpaperEntry* entry = wallpaper_registry_get_by_id(id);
    if (!entry) return false;
    
    // BMLE Ownership Fix: Never permanently retain 8.29MB RAW surfaces
    struct BOSImage* img = bopawn_load(entry->path);
    if (img && img->surface) {
        entry->width = img->width;
        entry->height = img->height;
        entry->loaded = true;
        g_current_wallpaper = entry;
        
        bool applied = _apply_wallpaper(img->surface);
        
        // Immediately release the unbounded RAW allocation from the cache
        image_cache_remove(entry->path);
        entry->cached_surface = NULL; // Ensure no stale pointers
        
        return applied;
    }
    
    return false;
}

bool wallpaper_set_path(const char* path) {
    struct BOSImage* img = bopawn_load(path);
    if (img && img->surface) {
        bool applied = _apply_wallpaper(img->surface);
        image_cache_remove(path);
        return applied;
    }
    return false;
}

WallpaperEntry* wallpaper_current(void) {
    return g_current_wallpaper;
}

void wallpaper_reload(void) {
    if (g_current_wallpaper) {
        wallpaper_set(g_current_wallpaper->id);
    }
}

void wallpaper_unload(void) {
    desktop_set_wallpaper(NULL);
    desktop_refresh_background();
}

void wallpaper_cache(void) {
    // Already handled by bopawn_load internally
}

void wallpaper_destroy(void) {
    wallpaper_unload();
}

WallpaperScaleMode wallpaper_get_scale_mode(void) {
    return g_current_scale_mode;
}

void wallpaper_set_scale_mode(WallpaperScaleMode mode) {
    g_current_scale_mode = mode;
    wallpaper_reload();
}
