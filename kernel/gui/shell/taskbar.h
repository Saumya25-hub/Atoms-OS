#ifndef GUI_TASKBAR_H
#define GUI_TASKBAR_H

#include "kernel/gui/surface/surface.h"
#include <stdbool.h>

#define TASKBAR_HEIGHT 40

// Initialize the taskbar surface
bool taskbar_init(struct BOSSurface* parent, int screen_width, int screen_height);

// Draw the taskbar contents
void taskbar_draw(const BVRect* clip);

// Process hover/click on the taskbar
bool taskbar_hit_test(int x, int y, int buttons);

// Get the taskbar surface
struct BOSSurface* taskbar_get_surface(void);

#endif // GUI_TASKBAR_H
