#include "kernel/gui/desktop/desktop_surface.h"
#include "kernel/gui/compositor/compositor.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/render/painter.h"
#include <stddef.h>

static struct BOSSurface* root_desktop = NULL;
static uint32_t desktop_bg_color = 0xFF008080; // Default teal

bool desktop_init(int width, int height) {
    if (root_desktop) return false;

    root_desktop = surface_create(width, height);
    if (!root_desktop) return false;

    root_desktop->visible = true;
    root_desktop->x = 0;
    root_desktop->y = 0;
    
    // Fill with default background color
    BVRect full = {0, 0, width, height};
    painter_fill_rect(root_desktop, &full, desktop_bg_color, NULL);
    
    // Initialize compositor with this root surface
    compositor_init(root_desktop);

    return true;
}

struct BOSSurface* desktop_get_root(void) {
    return root_desktop;
}

void desktop_set_color(uint32_t color) {
    desktop_bg_color = color;
    if (root_desktop) {
        BVRect full = {0, 0, root_desktop->width, root_desktop->height};
        painter_fill_rect(root_desktop, &full, desktop_bg_color, NULL);
        compositor_invalidate_surface(root_desktop);
    }
}
