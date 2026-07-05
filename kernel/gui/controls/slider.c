#include "kernel/gui/controls/slider.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/render/painter.h"

static void slider_paint(BOSControl* self, struct BOSSurface* surface, const BVRect* clip) {
    BOSSlider* sl = (BOSSlider*)self;
    if (!self->visible) return;
    
    int ax, ay;
    control_get_absolute_position(self, &ax, &ay);
    
    // Draw track
    int track_y = ay + self->height / 2 - 2;
    BVRect track_rect = {ax, track_y, self->width, 4};
    BOVISUAL_Color track_color = {100, 100, 100, 255};
    painter_fill_rect(surface, &track_rect, track_color, clip);
    
    // Draw thumb
    float pct = 0.0f;
    if (sl->max_val > sl->min_val) {
        pct = (float)(sl->value - sl->min_val) / (float)(sl->max_val - sl->min_val);
    }
    
    int thumb_w = 8;
    int thumb_x = ax + (int)((self->width - thumb_w) * pct);
    BVRect thumb_rect = {thumb_x, ay, thumb_w, self->height};
    
    BOVISUAL_Color thumb_color = sl->base.pressed ? (BOVISUAL_Color){150, 150, 150, 255} : (BOVISUAL_Color){200, 200, 200, 255};
    painter_fill_rect(surface, &thumb_rect, thumb_color, clip);
}

static void slider_update_value_from_mouse(BOSSlider* sl, int mouse_x) {
    int ax, ay;
    control_get_absolute_position(&sl->base, &ax, &ay);
    
    int thumb_w = 8;
    int track_w = sl->base.width - thumb_w;
    
    int rel_x = mouse_x - ax - (thumb_w / 2);
    if (rel_x < 0) rel_x = 0;
    if (rel_x > track_w) rel_x = track_w;
    
    float pct = (float)rel_x / (float)track_w;
    int new_val = sl->min_val + (int)(pct * (sl->max_val - sl->min_val));
    
    if (new_val != sl->value) {
        sl->value = new_val;
        control_invalidate(&sl->base);
        if (sl->on_value_changed) {
            sl->on_value_changed(&sl->base, sl->value);
        }
    }
}

static void slider_handle_event(BOSControl* self, const GUIEvent* event) {
    BOSSlider* sl = (BOSSlider*)self;
    if (!self->enabled) return;
    
    switch (event->type) {
        case GUI_EVENT_MOUSE_DOWN:
            if (event->data.mouse.buttons & 1) {
                self->pressed = true;
                sl->dragging = true;
                slider_update_value_from_mouse(sl, event->data.mouse.x);
            }
            break;
            
        case GUI_EVENT_MOUSE_MOVE:
            if (sl->dragging) {
                slider_update_value_from_mouse(sl, event->data.mouse.x);
            }
            break;
            
        case GUI_EVENT_MOUSE_UP:
            if (self->pressed) {
                self->pressed = false;
                sl->dragging = false;
                control_invalidate(self);
            }
            break;
            
        case GUI_EVENT_MOUSE_LEAVE:
            // Optional: stop dragging if left window? Usually we want global capture, 
            // but for simple GUI we stop dragging.
            self->pressed = false;
            sl->dragging = false;
            control_invalidate(self);
            break;
            
        default:
            break;
    }
}

static void slider_destroy(BOSControl* self) {
    kfree(self);
}

BOSSlider* slider_create() {
    BOSSlider* sl = (BOSSlider*)kmalloc(sizeof(BOSSlider));
    if (!sl) return NULL;
    
    control_init(&sl->base);
    sl->base.paint = slider_paint;
    sl->base.handle_event = slider_handle_event;
    sl->base.destroy = slider_destroy;
    
    sl->base.width = 100;
    sl->base.height = 16;
    
    sl->min_val = 0;
    sl->max_val = 100;
    sl->value = 0;
    sl->dragging = false;
    sl->on_value_changed = NULL;
    
    return sl;
}

void slider_set_range(BOSSlider* slider, int min_val, int max_val) {
    if (!slider) return;
    slider->min_val = min_val;
    slider->max_val = max_val;
    control_invalidate(&slider->base);
}

void slider_set_value(BOSSlider* slider, int value) {
    if (!slider) return;
    if (slider->value != value) {
        slider->value = value;
        control_invalidate(&slider->base);
    }
}
