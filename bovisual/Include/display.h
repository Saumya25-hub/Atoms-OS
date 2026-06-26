#ifndef BOVISUAL_DISPLAY_H
#define BOVISUAL_DISPLAY_H

#include <stdint.h>
#include "bovisual_types.h"

// ----------------------------------------------------
// Z-Order Management
// ----------------------------------------------------
typedef enum {
    BV_Z_DESKTOP,
    BV_Z_WALLPAPER,
    BV_Z_PANEL,
    BV_Z_WINDOW,
    BV_Z_DIALOG,
    BV_Z_TOOLTIP,
    BV_Z_CURSOR,
    BV_Z_MAX
} BVZLayer;

// ----------------------------------------------------
// Display Manager (Source of Truth)
// ----------------------------------------------------
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    
    float aspect_ratio;
    float ui_scale; // Relative to 1080p baseline
    
    BVRect desktop_bounds; // Absolute full screen
    BVRect safe_area;      // Inset from edges (overscan/padding)
    BVRect work_area;      // Safe area minus taskbar
    BVRect center_area;    // Center region of work area
    
    BVZLayer active_z_layer;
} BOSDisplayInfo;

// Exposed Display API
void BOS_Display_Init(uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp);
const BOSDisplayInfo* BOS_Display_Get(void);

// Region Getters
BVRect BOS_Display_GetDesktopRect(void);
BVRect BOS_Display_GetSafeArea(void);
BVRect BOS_Display_GetWorkArea(void);
BVRect BOS_Display_GetCenterArea(void);

#endif // BOVISUAL_DISPLAY_H
