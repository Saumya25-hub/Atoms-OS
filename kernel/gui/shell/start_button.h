#ifndef GUI_START_BUTTON_H
#define GUI_START_BUTTON_H

#include "kernel/gui/surface/surface.h"
#include <stdbool.h>

#define START_BUTTON_WIDTH  80
#define START_BUTTON_HEIGHT 32
#define START_BUTTON_MARGIN 4

// Initialize the start button (attaches it to the taskbar)
void start_button_init(struct BOSSurface* taskbar_surface);

// Draw the start button
void start_button_draw(const BVRect* clip);

// Handle mouse interaction (coordinates relative to taskbar)
void start_button_hit_test(int local_x, int local_y, int buttons);

#endif // GUI_START_BUTTON_H
