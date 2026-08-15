#ifndef WALLPAPER_SERVICE_H
#define WALLPAPER_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

/*
 * 🖼️ ATOMS OS Wallpaper Service V3.0
 * Zero-Heap Multi-Wallpaper Random Slideshow Engine (10 QOI Wallpapers)
 * 60-Second Auto-Rotation with 1.0s Smooth Non-Linear Cubic Cross-Fade.
 * Zero dynamic memory allocations during runtime rendering (1GB RAM safe).
 */

void            wallpaper_service_init(void);
void            wallpaper_service_update(uint64_t delta_ms);
void            wallpaper_service_select_random(void);
uint32_t        wallpaper_service_get_selected_id(void);
const uint32_t* wallpaper_service_get_canvas(void);
void            wallpaper_service_render(uint32_t* target_fb, uint32_t fb_width, uint32_t fb_height, uint32_t fb_stride);

#endif /* WALLPAPER_SERVICE_H */
