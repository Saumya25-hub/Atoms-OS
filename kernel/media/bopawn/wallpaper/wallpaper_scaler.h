#pragma once
#include <stdint.h>
#include "kernel/gui/surface/surface.h"

typedef enum {
    WALLPAPER_SCALE_STRETCH,
    WALLPAPER_SCALE_FIT,
    WALLPAPER_SCALE_FILL,
    WALLPAPER_SCALE_CENTER,
    WALLPAPER_SCALE_TILE
} WallpaperScaleMode;

struct BOSSurface* wallpaper_scaler_scale(struct BOSSurface* src, int target_w, int target_h, WallpaperScaleMode mode, uint32_t bg_color);
