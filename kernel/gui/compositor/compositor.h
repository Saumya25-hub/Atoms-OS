#ifndef GUI_COMPOSITOR_H
#define GUI_COMPOSITOR_H

#include "kernel/gui/surface/surface.h"
#include "kernel/gui/region/dirty_region.h"

// Initialize the compositor with the root desktop surface
void compositor_init(struct BOSSurface* root_surface);

// Invalidate a specific rectangle on screen
void compositor_invalidate_rect(const BVRect* rect);

// Invalidate an entire surface
void compositor_invalidate_surface(struct BOSSurface* surface);

// Perform a composition pass (should be called once per frame/tick)
void compositor_compose(void);

// Force a full redraw (bypasses dirty regions)
void compositor_force_redraw(void);

// Get compositor performance metrics (for telemetry)
int compositor_get_drawn_rects(void);
int compositor_get_composition_time_ms(void);

#endif // GUI_COMPOSITOR_H
