#include "../include/bwe_geometry.h"

void BWE_Geometry_GetDecorationMetrics(BWE_Window* win, int32_t* top, int32_t* bottom, int32_t* left, int32_t* right) {
    if (!win) return;
    
    // Borderless windows have no decorations
    if (win->flags & BWE_WINDOW_BORDERLESS) {
        if (top) *top = 0;
        if (bottom) *bottom = 0;
        if (left) *left = 0;
        if (right) *right = 0;
        return;
    }
    
    // Standard window: 5px border all around, plus 30px titlebar at top
    // Some controls (like buttons/labels/panels) are technically BWE_Window but don't have borders drawn unless they're top-level windows.
    // In ATOMS OS, BWE_TYPE_WINDOW represents a top-level window.
    if (win->type == BWE_TYPE_WINDOW && win->parent_id == BWE_DESKTOP_ID) {
        if (top) *top = 35; // 5px border + 30px titlebar
        if (bottom) *bottom = 5;
        if (left) *left = 5;
        if (right) *right = 5;
    } else {
        // Child controls don't have window manager decorations
        if (top) *top = 0;
        if (bottom) *bottom = 0;
        if (left) *left = 0;
        if (right) *right = 0;
    }
}

void BWE_Geometry_CalculateScreenBounds(BWE_Window* win, BWE_Rect* out_bounds) {
    if (!win || !out_bounds) return;
    *out_bounds = win->screen_bounds;
}

void BWE_Geometry_CalculateClientBounds(BWE_Window* win, BWE_Rect* out_bounds) {
    if (!win || !out_bounds) return;
    
    int32_t t = 0, b = 0, l = 0, r = 0;
    BWE_Geometry_GetDecorationMetrics(win, &t, &b, &l, &r);
    
    out_bounds->x = win->screen_bounds.x + l;
    out_bounds->y = win->screen_bounds.y + t;
    out_bounds->width = win->screen_bounds.width - (l + r);
    out_bounds->height = win->screen_bounds.height - (t + b);
    
    if (out_bounds->width < 0) out_bounds->width = 0;
    if (out_bounds->height < 0) out_bounds->height = 0;
}
