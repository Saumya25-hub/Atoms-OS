#ifndef WALLPAPER_SERVICE_H
#define WALLPAPER_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

/*
 * 🖼️ ATOMS OS Wallpaper Service
 * Scans available boot wallpapers, selects ONE randomly on boot,
 * and maintains the same cached wallpaper for the entire login session.
 * Zero dynamic memory allocations during runtime rendering.
 */

void     wallpaper_service_init(void);
void     wallpaper_service_select_random(void);
uint32_t wallpaper_service_get_selected_id(void);
const uint32_t* wallpaper_service_get_canvas(void);
void     wallpaper_service_render(uint32_t* target_fb, uint32_t fb_width, uint32_t fb_height, uint32_t fb_stride);

#endif /* WALLPAPER_SERVICE_H */
