#ifndef GUI_DESKTOP_ICONS_H
#define GUI_DESKTOP_ICONS_H

#include "kernel/gui/surface/surface.h"
#include <stdbool.h>

#define MAX_DESKTOP_ICONS 16

typedef void (*DesktopIconCallback)(void);

typedef struct {
    int id;
    int x;
    int y;
    char name[32];
    int icon_id;
    DesktopIconCallback on_click;
    bool is_selected;
    bool is_hovered;
} DesktopIcon;

// Initialize the desktop icon manager
void desktop_icons_init(struct BOSSurface* desktop_surface);

// Add an icon to the desktop
bool desktop_icon_add(int x, int y, const char* name, int icon_id, DesktopIconCallback callback);

// Draw all icons
void desktop_icons_draw(const BVRect* clip);

// Process hover/click on the desktop icons
bool desktop_icons_hit_test(int x, int y, int buttons);

#endif // GUI_DESKTOP_ICONS_H
