#pragma once
#include <stdint.h>
#include <stdbool.h>

struct BOSSurface;

typedef struct {
    uint32_t id;
    const char* name;
    const char* path;
    int width;
    int height;
    const char* format;
    bool loaded;
    struct BOSSurface* cached_surface;
} WallpaperEntry;

#define MAX_WALLPAPERS 10

void wallpaper_registry_init(void);
WallpaperEntry* wallpaper_registry_get_by_id(uint32_t id);
WallpaperEntry* wallpaper_registry_get_by_name(const char* name);
int wallpaper_registry_get_count(void);
WallpaperEntry* wallpaper_registry_get_all(void);
