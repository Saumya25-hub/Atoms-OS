#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "wallpaper_registry.h"
#include "wallpaper_scaler.h"

void wallpaper_manager_init(void);
bool wallpaper_load(const char* path);
bool wallpaper_set(uint32_t id);
bool wallpaper_set_path(const char* path);
WallpaperEntry* wallpaper_current(void);
void wallpaper_reload(void);
void wallpaper_unload(void);
void wallpaper_cache(void);
void wallpaper_destroy(void);
WallpaperScaleMode wallpaper_get_scale_mode(void);
void wallpaper_set_scale_mode(WallpaperScaleMode mode);
