#include "kernel/gui/controls/scrollbar.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/render/painter.h"

static void scrollbar_paint(BOSControl* self, struct BOSSurface* surface, const BVRect* clip) {
    BOSScrollBar* sb = (BOSScrollBar*)self;
    if (!self->visible) return;
    
    int ax, ay;
    control_get_absolute_position(self, &ax, &ay);
    
    BVRect rect = {ax, ay, self->width, self->height};
    BOVISUAL_Color bg_color = {240, 240, 240, 255};
    painter_fill_rect(surface, &rect, bg_color, clip);
    
    // Stub thumb rendering
    if (sb->horizontal) {
        BVRect thumb = {ax + 2, ay + 2, 20, self->height - 4};
        BOVISUAL_Color thumb_color = {180, 180, 180, 255};
        painter_fill_rect(surface, &thumb, thumb_color, clip);
    } else {
        BVRect thumb = {ax + 2, ay + 2, self->width - 4, 20};
        BOVISUAL_Color thumb_color = {180, 180, 180, 255};
        painter_fill_rect(surface, &thumb, thumb_color, clip);
    }
}

static void scrollbar_handle_event(BOSControl* self, const GUIEvent* event) {
    (void)self; (void)event; // Stub
}

static void scrollbar_destroy(BOSControl* self) {
    kfree(self);
}

BOSScrollBar* scrollbar_create(bool horizontal) {
    BOSScrollBar* sb = (BOSScrollBar*)kmalloc(sizeof(BOSScrollBar));
    if (!sb) return NULL;
    
    control_init(&sb->base);
    sb->base.paint = scrollbar_paint;
    sb->base.handle_event = scrollbar_handle_event;
    sb->base.destroy = scrollbar_destroy;
    
    sb->horizontal = horizontal;
    if (horizontal) {
        sb->base.width = 100;
        sb->base.height = 16;
    } else {
        sb->base.width = 16;
        sb->base.height = 100;
    }
    
    sb->min_val = 0;
    sb->max_val = 100;
    sb->value = 0;
    sb->page_size = 10;
    
    return sb;
}
