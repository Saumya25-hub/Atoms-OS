#ifndef GUI_START_MENU_H
#define GUI_START_MENU_H

#include "kernel/gui/surface/surface.h"
#include <stdbool.h>

typedef enum {
    START_MENU_HIDDEN,
    START_MENU_VISIBLE
} StartMenuState;

// Initialize the start menu
void start_menu_init(struct BOSSurface* desktop_root);

// Show the start menu
void start_menu_show(void);

// Hide the start menu
void start_menu_hide(void);

// Toggle visibility
void start_menu_toggle(void);

// Update/Render the start menu
void start_menu_render(const BVRect* clip);

// Hit testing
bool start_menu_hit_test(int screen_x, int screen_y, int buttons);

// Is visible?
bool start_menu_is_visible(void);

#endif // GUI_START_MENU_H
