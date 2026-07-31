#include <stddef.h>
#include "../include/bosfsr_controls.h"
#include "../include/bosfsr_graphics.h"

static void button_paint(BOSFSR_Control* self, const BVFramebuffer* fb, int parent_x, int parent_y) {
    int ax = parent_x + self->x;
    int ay = parent_y + self->y;

    uint32_t bg = self->back_color;
    if (self->state == BOSFSR_STATE_HOVERED) bg = 0xFF0086E6;
    else if (self->state == BOSFSR_STATE_PRESSED) bg = 0xFF005A9E;

    BWE_FillRect(fb, ax, ay, self->width, self->height, bg);
    BWE_DrawRect(fb, ax, ay, self->width, self->height, self->border_color, 1);
    BWE_DrawTextRole(fb, self->name, ax + 10, ay + (self->height - 12) / 2, self->fore_color, 0);
}

static void label_paint(BOSFSR_Control* self, const BVFramebuffer* fb, int parent_x, int parent_y) {
    int ax = parent_x + self->x;
    int ay = parent_y + self->y;
    BWE_DrawTextRole(fb, self->name, ax, ay, self->fore_color, 0);
}

static void checkbox_paint(BOSFSR_Control* self, const BVFramebuffer* fb, int parent_x, int parent_y) {
    int ax = parent_x + self->x;
    int ay = parent_y + self->y;

    int box_size = 16;
    int box_y = ay + (self->height - box_size) / 2;

    BWE_FillRect(fb, ax, box_y, box_size, box_size, 0xFF1E1E23);
    BWE_DrawRect(fb, ax, box_y, box_size, box_size, 0xFF0078D7, 1);

    if (self->state == BOSFSR_STATE_SELECTED) {
        BWE_DrawLine(fb, ax + 3, box_y + 8, ax + 6, box_y + 12, 0xFF0078D7);
        BWE_DrawLine(fb, ax + 6, box_y + 12, ax + 13, box_y + 4, 0xFF0078D7);
    }

    BWE_DrawTextRole(fb, self->name, ax + 22, ay + (self->height - 12) / 2, self->fore_color, 0);
}

static void radiobutton_paint(BOSFSR_Control* self, const BVFramebuffer* fb, int parent_x, int parent_y) {
    int ax = parent_x + self->x;
    int ay = parent_y + self->y;

    int size = 16;
    int ry = ay + (self->height - size) / 2;

    BWE_FillRect(fb, ax, ry, size, size, 0xFF1E1E23);
    BWE_DrawRect(fb, ax, ry, size, size, 0xFF0078D7, 1);

    if (self->state == BOSFSR_STATE_SELECTED) {
        BWE_FillRect(fb, ax + 4, ry + 4, 8, 8, 0xFF0078D7);
    }

    BWE_DrawTextRole(fb, self->name, ax + 22, ay + (self->height - 12) / 2, self->fore_color, 0);
}

static void progressbar_paint(BOSFSR_Control* self, const BVFramebuffer* fb, int parent_x, int parent_y) {
    int ax = parent_x + self->x;
    int ay = parent_y + self->y;

    BWE_FillRect(fb, ax, ay, self->width, self->height, 0xFF1E1E23);
    BWE_DrawRect(fb, ax, ay, self->width, self->height, 0xFF50505A, 1);

    int fill_w = (self->width - 4) * self->corner_radius / 100;
    if (fill_w > 0) {
        BWE_FillRect(fb, ax + 2, ay + 2, fill_w, self->height - 4, 0xFF0078D7);
    }
}

static void gridview_paint(BOSFSR_Control* self, const BVFramebuffer* fb, int parent_x, int parent_y) {
    int ax = parent_x + self->x;
    int ay = parent_y + self->y;

    BWE_FillRect(fb, ax, ay, self->width, self->height, 0xFF1E1E23);
    BWE_DrawRect(fb, ax, ay, self->width, self->height, 0xFF464650, 1);
    BWE_FillRect(fb, ax, ay, self->width, 25, 0xFF2D2D34);
    BWE_DrawTextRole(fb, "ID   Name      Status", ax + 10, ay + 5, 0xFFFFFFFF, 0);
    BWE_DrawTextRole(fb, "01   Item_1    Active", ax + 10, ay + 35, 0xFFCCCCCC, 0);
    BWE_DrawTextRole(fb, "02   Item_2    Active", ax + 10, ay + 65, 0xFFCCCCCC, 0);
}

static void image_paint(BOSFSR_Control* self, const BVFramebuffer* fb, int parent_x, int parent_y) {
    int ax = parent_x + self->x;
    int ay = parent_y + self->y;

    BWE_FillRect(fb, ax, ay, self->width, self->height, 0xFF232328);
    BWE_DrawRect(fb, ax, ay, self->width, self->height, 0xFF50505A, 1);
    BWE_DrawLine(fb, ax, ay, ax + self->width, ay + self->height, 0xFF50505A);
    BWE_DrawLine(fb, ax + self->width, ay, ax, ay + self->height, 0xFF50505A);
    BWE_DrawTextRole(fb, "IMAGE", ax + self->width / 3, ay + self->height / 2 - 6, 0xFFFFFFFF, 0);
}

BOSFSR_Control* bosfsr_create_button(const char* name, int x, int y, int w, int h, const char* text, uint32_t bg_color) {
    BOSFSR_Control* c = bosfsr_create_control(text ? text : name, x, y, w, h);
    if (!c) return NULL;
    c->back_color = bg_color ? bg_color : 0xFF0078D7;
    c->paint = button_paint;
    return c;
}

BOSFSR_Control* bosfsr_create_label(const char* name, int x, int y, int w, int h, const char* text, uint32_t fg_color) {
    BOSFSR_Control* c = bosfsr_create_control(text ? text : name, x, y, w, h);
    if (!c) return NULL;
    c->fore_color = fg_color ? fg_color : 0xFFFFFFFF;
    c->paint = label_paint;
    return c;
}

BOSFSR_Control* bosfsr_create_checkbox(const char* name, int x, int y, int w, int h, const char* text, bool is_checked) {
    BOSFSR_Control* c = bosfsr_create_control(text ? text : name, x, y, w, h);
    if (!c) return NULL;
    if (is_checked) c->state = BOSFSR_STATE_SELECTED;
    c->paint = checkbox_paint;
    return c;
}

BOSFSR_Control* bosfsr_create_radiobutton(const char* name, int x, int y, int w, int h, const char* text, bool is_checked) {
    BOSFSR_Control* c = bosfsr_create_control(text ? text : name, x, y, w, h);
    if (!c) return NULL;
    if (is_checked) c->state = BOSFSR_STATE_SELECTED;
    c->paint = radiobutton_paint;
    return c;
}

BOSFSR_Control* bosfsr_create_progressbar(const char* name, int x, int y, int w, int h, int value) {
    BOSFSR_Control* c = bosfsr_create_control(name, x, y, w, h);
    if (!c) return NULL;
    c->corner_radius = value;
    c->paint = progressbar_paint;
    return c;
}

BOSFSR_Control* bosfsr_create_gridview(const char* name, int x, int y, int w, int h) {
    BOSFSR_Control* c = bosfsr_create_control(name, x, y, w, h);
    if (!c) return NULL;
    c->paint = gridview_paint;
    return c;
}

BOSFSR_Control* bosfsr_create_image(const char* name, int x, int y, int w, int h) {
    BOSFSR_Control* c = bosfsr_create_control(name, x, y, w, h);
    if (!c) return NULL;
    c->paint = image_paint;
    return c;
}
