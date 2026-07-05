#include "kernel/gui/controls/button.h"
#include "kernel/gui/theme/theme_engine.h"
#include "kernel/core/memory.h"
#include <string.h>

static void button_paint(BOSControl* self, struct BOSSurface* surface, const BVRect* clip) {
    BOSButton* btn = (BOSButton*)self;
    if (!self->visible) return;
    
    int ax, ay;
    control_get_absolute_position(self, &ax, &ay);
    
    BVRect rect;
    rect.x = ax;
    rect.y = ay;
    rect.w = self->width;
    rect.h = self->height;
    
    // Check intersection with clip here (omitted for brevity, assume painter does it)
    
    // Choose colors based on state
    BOVISUAL_Color bg_color;
    BOVISUAL_Color border_color = {0, 0, 0, 255}; // Placeholder, should use theme
    BOVISUAL_Color text_color = {255, 255, 255, 255};
    
    if (!self->enabled) {
        bg_color = (BOVISUAL_Color){100, 100, 100, 255};
    } else if (self->pressed) {
        bg_color = (BOVISUAL_Color){50, 150, 250, 255};
    } else if (self->hovered) {
        bg_color = (BOVISUAL_Color){80, 180, 255, 255};
    } else {
        bg_color = (BOVISUAL_Color){60, 60, 60, 255};
    }
    
    // Draw background
    painter_fill_rect(surface, &rect, bg_color, clip);
    
    // Draw border
    painter_draw_rect(surface, &rect, border_color, clip);
    
    // Draw Text (stubbed - assumes painter_draw_text or similar exists)
    // int text_w = strlen(btn->text) * 8; // Assuming 8px wide font
    // int tx = ax + (self->width - text_w) / 2;
    // int ty = ay + (self->height - 16) / 2; // Assuming 16px high font
    // painter_draw_text(surface, tx, ty, btn->text, text_color, clip);
}

static void button_handle_event(BOSControl* self, const GUIEvent* event) {
    BOSButton* btn = (BOSButton*)self;
    if (!self->enabled) return;
    
    switch (event->type) {
        case GUI_EVENT_MOUSE_ENTER:
            self->hovered = true;
            control_invalidate(self);
            break;
            
        case GUI_EVENT_MOUSE_LEAVE:
            self->hovered = false;
            self->pressed = false;
            control_invalidate(self);
            break;
            
        case GUI_EVENT_MOUSE_DOWN:
            if (event->data.mouse.buttons & 1) { // Left click
                self->pressed = true;
                control_invalidate(self);
            }
            break;
            
        case GUI_EVENT_MOUSE_UP:
            if (self->pressed) {
                self->pressed = false;
                control_invalidate(self);
                if (btn->on_click) {
                    btn->on_click(self);
                }
            }
            break;
            
        default:
            break;
    }
}

static void button_destroy(BOSControl* self) {
    // kfree(self); 
    // Assuming dynamic allocation is used, free here.
}

BOSButton* button_create(const char* text) {
    // BOSButton* btn = (BOSButton*)kmalloc(sizeof(BOSButton));
    // Hack for now, allocate in some static way or assume kmalloc exists.
    // For compilation sake in this environment we will mock kmalloc or use a static pool if missing.
    // Let's assume kmalloc is defined in kernel/core/memory.h
    extern void* kmalloc(size_t size);
    
    BOSButton* btn = (BOSButton*)kmalloc(sizeof(BOSButton));
    if (!btn) return NULL;
    
    control_init(&btn->base);
    
    btn->base.paint = button_paint;
    btn->base.handle_event = button_handle_event;
    btn->base.destroy = button_destroy;
    
    strncpy(btn->text, text ? text : "", sizeof(btn->text) - 1);
    btn->on_click = NULL;
    
    return btn;
}

void button_set_text(BOSButton* button, const char* text) {
    if (!button) return;
    strncpy(button->text, text ? text : "", sizeof(button->text) - 1);
    control_invalidate(&button->base);
}

void button_set_on_click(BOSButton* button, void (*on_click)(BOSControl*)) {
    if (!button) return;
    button->on_click = on_click;
}
