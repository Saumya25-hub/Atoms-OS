#include "kernel/gui/controls/progressbar.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/render/painter.h"

static void progressbar_paint(BOSControl* self, struct BOSSurface* surface, const BVRect* clip) {
    BOSProgressBar* pb = (BOSProgressBar*)self;
    if (!self->visible) return;
    
    int ax, ay;
    control_get_absolute_position(self, &ax, &ay);
    
    BVRect rect = {ax, ay, self->width, self->height};
    
    // Draw background
    BOVISUAL_Color bg_color = {40, 40, 40, 255};
    painter_fill_rect(surface, &rect, bg_color, clip);
    
    // Draw border
    BOVISUAL_Color border_color = {100, 100, 100, 255};
    painter_draw_rect(surface, &rect, border_color, clip);
    
    // Draw progress fill
    if (pb->max_val > pb->min_val && pb->value > pb->min_val) {
        float pct = (float)(pb->value - pb->min_val) / (float)(pb->max_val - pb->min_val);
        if (pct > 1.0f) pct = 1.0f;
        
        int fill_w = (int)((self->width - 2) * pct);
        if (fill_w > 0) {
            BVRect fill_rect = {ax + 1, ay + 1, fill_w, self->height - 2};
            BOVISUAL_Color fill_color = {50, 180, 50, 255}; // Greenish
            painter_fill_rect(surface, &fill_rect, fill_color, clip);
        }
    }
}

static void progressbar_handle_event(BOSControl* self, const GUIEvent* event) {
    // Progress bars typically don't take input
    (void)self;
    (void)event;
}

static void progressbar_destroy(BOSControl* self) {
    kfree(self);
}

BOSProgressBar* progressbar_create() {
    BOSProgressBar* pb = (BOSProgressBar*)kmalloc(sizeof(BOSProgressBar));
    if (!pb) return NULL;
    
    control_init(&pb->base);
    pb->base.paint = progressbar_paint;
    pb->base.handle_event = progressbar_handle_event;
    pb->base.destroy = progressbar_destroy;
    
    pb->base.width = 150;
    pb->base.height = 16;
    
    pb->min_val = 0;
    pb->max_val = 100;
    pb->value = 0;
    
    return pb;
}

void progressbar_set_range(BOSProgressBar* pb, int min_val, int max_val) {
    if (!pb) return;
    pb->min_val = min_val;
    pb->max_val = max_val;
    control_invalidate(&pb->base);
}

void progressbar_set_value(BOSProgressBar* pb, int value) {
    if (!pb) return;
    if (pb->value != value) {
        pb->value = value;
        control_invalidate(&pb->base);
    }
}
