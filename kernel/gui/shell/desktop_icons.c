#include "desktop_icons.h"
#include "kernel/gui/theme/theme_engine.h"
#include "kernel/gui/compositor/compositor.h"
#include <string.h>

static DesktopIcon icons[MAX_DESKTOP_ICONS];
static int icon_count = 0;
static struct BOSSurface* parent_desktop = NULL;

static void _strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    if (i < n) dest[i] = '\0';
    else if (n > 0) dest[n - 1] = '\0';
}

void desktop_icons_init(struct BOSSurface* desktop_surface) {
    parent_desktop = desktop_surface;
    icon_count = 0;
}

bool desktop_icon_add(int x, int y, const char* name, int icon_id, DesktopIconCallback callback) {
    if (icon_count >= MAX_DESKTOP_ICONS) return false;
    
    DesktopIcon* icon = &icons[icon_count++];
    icon->id = icon_count;
    icon->x = x;
    icon->y = y;
    _strncpy(icon->name, name, sizeof(icon->name));
    icon->icon_id = icon_id;
    icon->on_click = callback;
    icon->is_selected = false;
    icon->is_hovered = false;
    
    return true;
}

void desktop_icons_draw(const BVRect* clip) {
    if (!parent_desktop) return;
    
    for (int i = 0; i < icon_count; i++) {
        DesktopIcon* icon = &icons[i];
        
        // Simple bounding box check before drawing
        BVRect icon_rect = {icon->x, icon->y, 64, 64};
        if (clip) {
            BVRect intersect;
            if (!dirty_region_clip(&intersect, &icon_rect, clip)) {
                continue; // Outside clip rect
            }
        }
        
        theme_draw_icon(parent_desktop, icon->x, icon->y, icon->name, icon->icon_id, icon->is_selected, icon->is_hovered, clip);
    }
}

bool desktop_icons_hit_test(int x, int y, int buttons) {
    if (!parent_desktop) return false;
    
    bool handled = false;
    bool left_click = (buttons & 1);
    
    for (int i = 0; i < icon_count; i++) {
        DesktopIcon* icon = &icons[i];
        bool is_inside = (x >= icon->x && x < icon->x + 64 && y >= icon->y && y < icon->y + 64);
        
        bool old_hover = icon->is_hovered;
        bool old_selected = icon->is_selected;
        
        icon->is_hovered = is_inside;
        
        if (is_inside && left_click) {
            icon->is_selected = true;
            handled = true;
            
            // Double click logic could be handled here or by event queue router
            // if (double_click && icon->on_click) icon->on_click();
        } else if (left_click && !is_inside) {
            icon->is_selected = false;
        }
        
        if (old_hover != icon->is_hovered || old_selected != icon->is_selected) {
            BVRect icon_rect = {parent_desktop->x + icon->x, parent_desktop->y + icon->y, 64, 64};
            compositor_invalidate_rect(&icon_rect);
        }
        
        if (is_inside) handled = true;
    }
    
    return handled;
}
