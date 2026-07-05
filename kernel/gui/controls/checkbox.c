#include "kernel/gui/controls/checkbox.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/render/painter.h"
#include <string.h>

static void checkbox_paint(BOSControl* self, struct BOSSurface* surface, const BVRect* clip) {
    BOSCheckBox* cb = (BOSCheckBox*)self;
    if (!self->visible) return;
    
    int ax, ay;
    control_get_absolute_position(self, &ax, &ay);
    
    // Draw box
    BVRect box_rect = {ax, ay + (self->height - 12)/2, 12, 12};
    BOVISUAL_Color bg_color = self->pressed ? (BOVISUAL_Color){100, 100, 100, 255} : (BOVISUAL_Color){255, 255, 255, 255};
    painter_fill_rect(surface, &box_rect, bg_color, clip);
    
    BOVISUAL_Color border_color = {50, 50, 50, 255};
    painter_draw_rect(surface, &box_rect, border_color, clip);
    
    // Draw check
    if (cb->checked) {
        BOVISUAL_Color check_color = {0, 0, 0, 255};
        painter_draw_line(surface, box_rect.x + 2, box_rect.y + 6, box_rect.x + 5, box_rect.y + 9, check_color, clip);
        painter_draw_line(surface, box_rect.x + 5, box_rect.y + 9, box_rect.x + 10, box_rect.y + 3, check_color, clip);
    }
    
    // Draw Text (stub)
    // BOVISUAL_Color text_color = {0, 0, 0, 255};
    // int tx = ax + 16;
    // int ty = ay + (self->height - 16) / 2;
    // painter_draw_text(surface, tx, ty, cb->text, text_color, clip);
}

static void checkbox_handle_event(BOSControl* self, const GUIEvent* event) {
    BOSCheckBox* cb = (BOSCheckBox*)self;
    if (!self->enabled) return;
    
    switch (event->type) {
        case GUI_EVENT_MOUSE_DOWN:
            if (event->data.mouse.buttons & 1) {
                self->pressed = true;
                control_invalidate(self);
            }
            break;
            
        case GUI_EVENT_MOUSE_UP:
            if (self->pressed) {
                self->pressed = false;
                cb->checked = !cb->checked;
                control_invalidate(self);
                if (cb->on_toggle) {
                    cb->on_toggle(self, cb->checked);
                }
            }
            break;
            
        case GUI_EVENT_MOUSE_LEAVE:
            self->pressed = false;
            control_invalidate(self);
            break;
            
        default:
            break;
    }
}

static void checkbox_destroy(BOSControl* self) {
    kfree(self);
}

BOSCheckBox* checkbox_create(const char* text) {
    BOSCheckBox* cb = (BOSCheckBox*)kmalloc(sizeof(BOSCheckBox));
    if (!cb) return NULL;
    
    control_init(&cb->base);
    cb->base.paint = checkbox_paint;
    cb->base.handle_event = checkbox_handle_event;
    cb->base.destroy = checkbox_destroy;
    
    cb->base.width = 100;
    cb->base.height = 16;
    
    strncpy(cb->text, text ? text : "", sizeof(cb->text) - 1);
    cb->checked = false;
    cb->on_toggle = NULL;
    
    return cb;
}

void checkbox_set_checked(BOSCheckBox* checkbox, bool checked) {
    if (!checkbox) return;
    if (checkbox->checked != checked) {
        checkbox->checked = checked;
        control_invalidate(&checkbox->base);
    }
}

bool checkbox_is_checked(BOSCheckBox* checkbox) {
    return checkbox ? checkbox->checked : false;
}
