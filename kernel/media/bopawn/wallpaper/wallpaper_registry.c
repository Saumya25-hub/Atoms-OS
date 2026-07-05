#include "wallpaper_registry.h"
#include "kernel/core/lib/include/string.h"
#include <stddef.h>

static WallpaperEntry g_wallpapers[MAX_WALLPAPERS];
static int g_wallpaper_count = 0;

void wallpaper_registry_init(void) {
    g_wallpaper_count = 5;
    
    g_wallpapers[0] = (WallpaperEntry){1, "Nature", "/W1.PNG", 0, 0, "PNG", false, NULL};
    g_wallpapers[1] = (WallpaperEntry){2, "Galaxy", "/W2.PNG", 0, 0, "PNG", false, NULL};
    g_wallpapers[2] = (WallpaperEntry){3, "Water", "/W3.PNG", 0, 0, "PNG", false, NULL};
    g_wallpapers[3] = (WallpaperEntry){4, "Mountains", "/W4.PNG", 0, 0, "PNG", false, NULL};
    g_wallpapers[4] = (WallpaperEntry){5, "Minimal", "/W5.PNG", 0, 0, "PNG", false, NULL};
}

WallpaperEntry* wallpaper_registry_get_by_id(uint32_t id) {
    for (int i = 0; i < g_wallpaper_count; i++) {
        if (g_wallpapers[i].id == id) return &g_wallpapers[i];
    }
    return NULL;
}

WallpaperEntry* wallpaper_registry_get_by_name(const char* name) {
    for (int i = 0; i < g_wallpaper_count; i++) {
        if (strcmp(g_wallpapers[i].name, name) == 0) return &g_wallpapers[i];
    }
    return NULL;
}

int wallpaper_registry_get_count(void) {
    return g_wallpaper_count;
}

WallpaperEntry* wallpaper_registry_get_all(void) {
    return g_wallpapers;
}
