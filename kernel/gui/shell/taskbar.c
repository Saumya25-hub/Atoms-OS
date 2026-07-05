#include "taskbar.h"
#include "start_button.h"
#include "kernel/gui/theme/theme_engine.h"
#include "kernel/gui/compositor/compositor.h"
#include <stddef.h>

static struct BOSSurface* taskbar_surface = NULL;

bool taskbar_init(struct BOSSurface* parent, int screen_width, int screen_height) {
    if (taskbar_surface) return false;
    if (!parent) return false;

    taskbar_surface = surface_create(screen_width, TASKBAR_HEIGHT);
    if (!taskbar_surface) return false;

    taskbar_surface->x = 0;
    taskbar_surface->y = screen_height - TASKBAR_HEIGHT;
    taskbar_surface->visible = true;

    // The taskbar is a child of the desktop, but needs to be above icons and below windows ideally,
    // or always on top. For now, it's a child of desktop root.
    surface_add_child(parent, taskbar_surface);

    // Initialize start button
    start_button_init(taskbar_surface);

    taskbar_draw(NULL);
    return true;
}

void taskbar_draw(const BVRect* clip) {
    if (!taskbar_surface) return;
    
    // Draw background
    theme_draw_taskbar(taskbar_surface, clip);
    
    // Draw Start Button
    start_button_draw(clip);
    
    // Future: Draw Running Applications Area, Clock, Status Area
    
    compositor_invalidate_surface(taskbar_surface);
}

bool taskbar_hit_test(int x, int y, int buttons) {
    if (!taskbar_surface || !taskbar_surface->visible) return false;

    // Check if within taskbar bounds
    if (x >= taskbar_surface->x && x < taskbar_surface->x + taskbar_surface->width &&
        y >= taskbar_surface->y && y < taskbar_surface->y + taskbar_surface->height) {
        
        // Convert to local coordinates for child components
        int local_x = x - taskbar_surface->x;
        int local_y = y - taskbar_surface->y;
        
        // Pass to start button
        start_button_hit_test(local_x, local_y, buttons);
        
        return true; // Handled by taskbar
    }
    return false;
}

struct BOSSurface* taskbar_get_surface(void) {
    return taskbar_surface;
}
