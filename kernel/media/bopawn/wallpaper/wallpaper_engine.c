#include "kernel/media/bopawn/bopawn.h"
#include "kernel/gui/desktop/desktop_surface.h"
#include <stddef.h>


static BOSImage* current_wallpaper = NULL;

void wallpaper_set(const char* path) {
    if (!path) return;
    
    BOSImage* img = bopawn_load(path);
    if (!img) return;
    
    if (current_wallpaper) {
        bopawn_destroy(current_wallpaper);
    }
    
    current_wallpaper = img;
    
    // In ATOMS OS, desktop_surface_set_wallpaper or similar would be called.
    // For now, we get the surface and attach it to the root desktop surface if it exists.
    // struct BOSSurface* root = desktop_get_root();
    // if (root) {
        // Redraw desktop background with this surface
        // Ideally handled via compositor or painter
        // extern void desktop_set_background_surface(struct BOSSurface* surface);
        // desktop_set_background_surface(current_wallpaper->surface);
    // }
}

struct BOSSurface* wallpaper_get(void) {
    if (!current_wallpaper) return NULL;
    return bopawn_get_surface(current_wallpaper);
}
