#include <stddef.h>
#include "../include/bosfsr_containers.h"
#include "../include/bosfsr_graphics.h"

static void panel_paint(BOSFSR_Control* self, const BVFramebuffer* fb, int parent_x, int parent_y) {
    int ax = parent_x + self->x;
    int ay = parent_y + self->y;

    if (self->has_shadow) {
        bosfsr_draw_shadow(fb, ax, ay, self->width, self->height, 4);
    }

    if (self->is_gradient) {
        bosfsr_draw_linear_gradient(fb, ax, ay, self->width, self->height, self->start_color, self->end_color, true);
    } else {
        bosfsr_draw_rounded_rect(fb, ax, ay, self->width, self->height, self->corner_radius, self->back_color, self->border_color, self->border_width);
    }
}

BOSFSR_Control* bosfsr_create_panel(const char* name, int x, int y, int w, int h, uint32_t bg_color) {
    BOSFSR_Control* c = bosfsr_create_control(name, x, y, w, h);
    if (!c) return NULL;
    c->back_color = bg_color;
    c->paint = panel_paint;
    return c;
}

BOSFSR_Control* bosfsr_create_rounded_panel(const char* name, int x, int y, int w, int h, int radius, uint32_t bg_color, uint32_t border_color, int border_w) {
    BOSFSR_Control* c = bosfsr_create_control(name, x, y, w, h);
    if (!c) return NULL;
    c->back_color = bg_color;
    c->border_color = border_color;
    c->border_width = border_w;
    c->corner_radius = radius;
    c->paint = panel_paint;
    return c;
}

BOSFSR_Control* bosfsr_create_gradient_panel(const char* name, int x, int y, int w, int h, uint32_t start_col, uint32_t end_col, bool vertical) {
    BOSFSR_Control* c = bosfsr_create_control(name, x, y, w, h);
    if (!c) return NULL;
    c->is_gradient = true;
    c->start_color = start_col;
    c->end_color = end_col;
    c->paint = panel_paint;
    return c;
}
