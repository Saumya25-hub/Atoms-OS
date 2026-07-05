#ifndef GUI_DESKTOP_SURFACE_H
#define GUI_DESKTOP_SURFACE_H

#include "kernel/gui/surface/surface.h"
#include <stdbool.h>

// Initialize the root desktop surface with screen dimensions
bool desktop_init(int width, int height);

// Get the root desktop surface
struct BOSSurface* desktop_get_root(void);

// Set the desktop background color
void desktop_set_color(uint32_t color);

#endif // GUI_DESKTOP_SURFACE_H
