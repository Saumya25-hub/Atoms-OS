#ifndef GUI_DESKTOP_SHELL_H
#define GUI_DESKTOP_SHELL_H

#include "kernel/gui/surface/surface.h"
#include <stdbool.h>

// Bootstraps the entire desktop environment
bool desktop_shell_init(int screen_width, int screen_height);

// Render pass for shell elements (called by compositor or separately)
void desktop_shell_render(const BVRect* clip);

// Receives events from interaction engine before windows
bool desktop_shell_hit_test(int screen_x, int screen_y, int buttons);

// Graceful shutdown
void desktop_shell_shutdown(void);

// Launcher API
bool desktop_launch_application(int app_id);

#endif // GUI_DESKTOP_SHELL_H
