#include "kernel/gui/controls/label.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/render/painter.h"
#include <string.h>

static void label_paint(BOSControl* self, struct BOSSurface* surface, const BVRect* clip) {
    BOSLabel* lbl = (BOSLabel*)self;
    if (!self->visible) return;
    
    int ax, ay;
    control_get_absolute_position(self, &ax, &ay);
    
    // BOVISUAL_Color text_color = {255, 255, 255, 255};
    // painter_draw_text(surface, ax, ay, lbl->text, text_color, clip);
}

static void label_handle_event(BOSControl* self, const GUIEvent* event) {
    // Labels generally do not handle events
    (void)self;
    (void)event;
}

static void label_destroy(BOSControl* self) {
    kfree(self);
}

BOSLabel* label_create(const char* text) {
    BOSLabel* lbl = (BOSLabel*)kmalloc(sizeof(BOSLabel));
    if (!lbl) return NULL;
    
    control_init(&lbl->base);
    lbl->base.paint = label_paint;
    lbl->base.handle_event = label_handle_event;
    lbl->base.destroy = label_destroy;
    
    lbl->base.width = 100;
    lbl->base.height = 16;
    
    strncpy(lbl->text, text ? text : "", sizeof(lbl->text) - 1);
    
    return lbl;
}

void label_set_text(BOSLabel* label, const char* text) {
    if (!label) return;
    strncpy(label->text, text ? text : "", sizeof(label->text) - 1);
    control_invalidate(&label->base);
}
