#include "kernel/gui/controls/radiobutton.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/render/painter.h"
#include <string.h>

static void radiobutton_paint(BOSControl* self, struct BOSSurface* surface, const BVRect* clip) {
    BOSRadioButton* rb = (BOSRadioButton*)self;
    if (!self->visible) return;
    
    int ax, ay;
    control_get_absolute_position(self, &ax, &ay);
    
    // Draw outer circle/box (using rect for now since we don't have painter_draw_circle)
    BVRect box_rect = {ax, ay + (self->height - 12)/2, 12, 12};
    BOVISUAL_Color bg_color = self->pressed ? (BOVISUAL_Color){100, 100, 100, 255} : (BOVISUAL_Color){255, 255, 255, 255};
    painter_fill_rect(surface, &box_rect, bg_color, clip);
    
    BOVISUAL_Color border_color = {50, 50, 50, 255};
    painter_draw_rect(surface, &box_rect, border_color, clip);
    
    // Draw selection indicator (inner rect)
    if (rb->selected) {
        BVRect inner_rect = {ax + 3, ay + (self->height - 12)/2 + 3, 6, 6};
        BOVISUAL_Color sel_color = {0, 0, 0, 255};
        painter_fill_rect(surface, &inner_rect, sel_color, clip);
    }
}

static void radiobutton_handle_event(BOSControl* self, const GUIEvent* event) {
    BOSRadioButton* rb = (BOSRadioButton*)self;
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
                
                // Only act if we are not already selected
                if (!rb->selected) {
                    // Deselect others in group
                    if (self->parent) {
                        BOSControl* curr = self->parent->first_child;
                        while (curr) {
                            if (curr != self && curr->paint == radiobutton_paint) { // Simple RTTI check
                                BOSRadioButton* sibling = (BOSRadioButton*)curr;
                                if (sibling->group_id == rb->group_id && sibling->selected) {
                                    sibling->selected = false;
                                    control_invalidate(curr);
                                }
                            }
                            curr = curr->next_sibling;
                        }
                    }
                    
                    rb->selected = true;
                    control_invalidate(self);
                    if (rb->on_select) {
                        rb->on_select(self);
                    }
                } else {
                    control_invalidate(self); // Just to clear pressed state visually
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

static void radiobutton_destroy(BOSControl* self) {
    kfree(self);
}

BOSRadioButton* radiobutton_create(const char* text, int group_id) {
    BOSRadioButton* rb = (BOSRadioButton*)kmalloc(sizeof(BOSRadioButton));
    if (!rb) return NULL;
    
    control_init(&rb->base);
    rb->base.paint = radiobutton_paint;
    rb->base.handle_event = radiobutton_handle_event;
    rb->base.destroy = radiobutton_destroy;
    
    rb->base.width = 100;
    rb->base.height = 16;
    
    strncpy(rb->text, text ? text : "", sizeof(rb->text) - 1);
    rb->selected = false;
    rb->group_id = group_id;
    rb->on_select = NULL;
    
    return rb;
}

void radiobutton_set_selected(BOSRadioButton* radiobutton, bool selected) {
    if (!radiobutton) return;
    if (radiobutton->selected != selected) {
        radiobutton->selected = selected;
        control_invalidate(&radiobutton->base);
    }
}
