#include "../Include/boscal.h"
#include "../Include/display.h"

// Singleton Display Instance
static BOSDisplayInfo g_display;

void BOS_Display_Init(uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp) {
    g_display.width = width;
    g_display.height = height;
    g_display.pitch = pitch;
    g_display.bpp = bpp;
    
    // Calculate aspect ratio (float for internal representation if needed, but we keep it simple)
    g_display.aspect_ratio = (float)width / (float)height;
    
    // Base scale on 1080p
    g_display.ui_scale = (float)height / 1080.0f;
    if (g_display.ui_scale < 0.5f) g_display.ui_scale = 0.5f;
    
    // Setup Virtual Desktop Rects
    g_display.desktop_bounds = (BVRect){0, 0, width, height};
    
    // 5% Safe Area padding
    int32_t safe_pad_x = (width * 5) / 100;
    int32_t safe_pad_y = (height * 5) / 100;
    g_display.safe_area = (BVRect){safe_pad_x, safe_pad_y, width - (safe_pad_x*2), height - (safe_pad_y*2)};
    
    // Work Area = Safe Area (Future: minus taskbar at bottom)
    g_display.work_area = g_display.safe_area;
    
    // Center Area = 60% of Work Area centered
    int32_t cw = (g_display.work_area.width * 60) / 100;
    int32_t ch = (g_display.work_area.height * 60) / 100;
    g_display.center_area = (BVRect){
        g_display.work_area.x + (g_display.work_area.width - cw) / 2,
        g_display.work_area.y + (g_display.work_area.height - ch) / 2,
        cw, ch
    };
    
    g_display.active_z_layer = BV_Z_DESKTOP;
}

const BOSDisplayInfo* BOS_Display_Get(void) { return &g_display; }
BVRect BOS_Display_GetDesktopRect(void) { return g_display.desktop_bounds; }
BVRect BOS_Display_GetSafeArea(void) { return g_display.safe_area; }
BVRect BOS_Display_GetWorkArea(void) { return g_display.work_area; }
BVRect BOS_Display_GetCenterArea(void) { return g_display.center_area; }

void BOSCAL_Init(uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp) {
    BOS_Display_Init(width, height, pitch, bpp);
}

void BOSCAL_OnDisplayChanged(uint32_t new_w, uint32_t new_h) {
    // In V1, we just re-init. In future, broadcast layout invalidate.
    BOSCAL_Init(new_w, new_h, g_display.pitch, g_display.bpp);
}

// Internal helper to convert abstract BVDimension to pixels
static int32_t ResolveDim(int32_t parent_size, int32_t viewport_size, BVDimension dim) {
    switch (dim.type) {
        case BV_UNIT_PX:      return dim.value;
        case BV_UNIT_PERCENT: return (parent_size * dim.value) / 100;
        case BV_UNIT_VW:      return (g_display.width * dim.value) / 100;
        case BV_UNIT_VH:      return (g_display.height * dim.value) / 100;
        case BV_UNIT_FILL:    return parent_size;
        case BV_UNIT_AUTO:    return dim.value; // Fallback to raw value if auto size isn't computed yet
        default:              return 0;
    }
}

BVRect BOSCAL_ResolveLayout(BVRect parent, BVDimension w, BVDimension h, BVAnchor anchor) {
    BVRect result;
    
    // 1. Resolve sizes
    result.width = ResolveDim(parent.width, g_display.width, w);
    result.height = ResolveDim(parent.height, g_display.height, h);
    
    // 2. Resolve anchor positions
    switch (anchor) {
        case BV_ANCHOR_CENTER:
            result.x = parent.x + (parent.width - result.width) / 2;
            result.y = parent.y + (parent.height - result.height) / 2;
            break;
            
        case BV_ANCHOR_TOP:
            result.x = parent.x + (parent.width - result.width) / 2;
            result.y = parent.y;
            break;
            
        case BV_ANCHOR_BOTTOM:
            result.x = parent.x + (parent.width - result.width) / 2;
            result.y = parent.y + parent.height - result.height;
            break;
            
        case BV_ANCHOR_LEFT:
            result.x = parent.x;
            result.y = parent.y + (parent.height - result.height) / 2;
            break;
            
        case BV_ANCHOR_RIGHT:
            result.x = parent.x + parent.width - result.width;
            result.y = parent.y + (parent.height - result.height) / 2;
            break;
            
        case BV_ANCHOR_STRETCH: // Stretch horizontally, center vertically
            result.x = parent.x;
            result.width = parent.width;
            result.y = parent.y + (parent.height - result.height) / 2;
            break;
            
        case BV_ANCHOR_FILL:
            result.x = parent.x;
            result.y = parent.y;
            result.width = parent.width;
            result.height = parent.height;
            break;
            
        case BV_ANCHOR_NONE:
        default:
            result.x = parent.x;
            result.y = parent.y;
            break;
    }
    
    return result;
}

BVRect BOSCAL_ResolveDesktopLayout(BVDimension w, BVDimension h, BVAnchor anchor) {
    return BOSCAL_ResolveLayout(BOS_Display_GetWorkArea(), w, h, anchor);
}
