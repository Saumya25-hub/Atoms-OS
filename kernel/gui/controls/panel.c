#include "kernel/gui/controls/panel.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/render/painter.h"
#include <stddef.h>

static void panel_paint(BOSControl* self, struct BOSSurface* surface, const BVRect* clip) {
    BOSPanel* pnl = (BOSPanel*)self;
    if (!self->visible) return;
    
    int ax, ay;
    control_get_absolute_position(self, &ax, &ay);
    
    if (!pnl->transparent) {
        BVRect rect = {ax, ay, self->width, self->height};
        BOVISUAL_Color bg_color = {220, 220, 220, 255}; // Default panel color
        painter_fill_rect(surface, &rect, bg_color, clip);
    }
    
    // Panel simply draws all its children
    control_paint_children(self, surface, clip);
}

static void panel_handle_event(BOSControl* self, const GUIEvent* event) {
    // Panels don't usually handle events themselves, except maybe for drag-and-drop
    // Event routing to children is handled by the window manager or interaction engine
    (void)self;
    (void)event;
}

static void panel_destroy(BOSControl* self) {
    // control_destroy_recursive will handle children first
    kfree(self);
}

BOSPanel* panel_create() {
    BOSPanel* pnl = (BOSPanel*)kmalloc(sizeof(BOSPanel));
    if (!pnl) return NULL;
    
    control_init(&pnl->base);
    pnl->base.paint = panel_paint;
    pnl->base.handle_event = panel_handle_event;
    pnl->base.destroy = panel_destroy;
    
    pnl->base.width = 200;
    pnl->base.height = 200;
    
    pnl->transparent = false;
    
    return pnl;
}

void panel_set_transparent(BOSPanel* panel, bool transparent) {
    if (!panel) return;
    panel->transparent = transparent;
    control_invalidate(&panel->base);
}
