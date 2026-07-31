#include "../include/bosfsr_core.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);

BOSFSR_Control* bosfsr_create_control(const char* name, int x, int y, int w, int h) {
    BOSFSR_Control* ctrl = (BOSFSR_Control*)kmalloc(sizeof(BOSFSR_Control));
    if (!ctrl) return NULL;
    memset(ctrl, 0, sizeof(BOSFSR_Control));

    if (name) strncpy(ctrl->name, name, sizeof(ctrl->name) - 1);
    ctrl->x = x;
    ctrl->y = y;
    ctrl->width = w;
    ctrl->height = h;
    ctrl->visible = true;
    ctrl->enabled = true;
    ctrl->back_color = 0xFF1E1E23;
    ctrl->fore_color = 0xFFFFFFFF;
    ctrl->border_color = 0xFF404045;
    ctrl->state = BOSFSR_STATE_NORMAL;

    return ctrl;
}

void bosfsr_add_child(BOSFSR_Control* parent, BOSFSR_Control* child) {
    if (!parent || !child) return;

    if (parent->child_count >= parent->child_capacity) {
        uint32_t new_cap = parent->child_capacity == 0 ? 8 : parent->child_capacity * 2;
        BOSFSR_Control** new_arr = (BOSFSR_Control**)kmalloc(sizeof(BOSFSR_Control*) * new_cap);
        if (!new_arr) return;

        if (parent->children) {
            memcpy(new_arr, parent->children, sizeof(BOSFSR_Control*) * parent->child_count);
            kfree(parent->children);
        }
        parent->children = new_arr;
        parent->child_capacity = new_cap;
    }

    child->parent = parent;
    parent->children[parent->child_count++] = child;
}

void bosfsr_destroy_control(BOSFSR_Control* ctrl) {
    if (!ctrl) return;
    for (uint32_t i = 0; i < ctrl->child_count; i++) {
        bosfsr_destroy_control(ctrl->children[i]);
    }
    if (ctrl->children) kfree(ctrl->children);
    kfree(ctrl);
}

void bosfsr_paint_tree(BOSFSR_Control* root, const BVFramebuffer* fb, int parent_x, int parent_y) {
    if (!root || !root->visible || !fb) return;

    int abs_x = parent_x + root->x;
    int abs_y = parent_y + root->y;

    if (root->paint) {
        root->paint(root, fb, parent_x, parent_y);
    }

    for (uint32_t i = 0; i < root->child_count; i++) {
        bosfsr_paint_tree(root->children[i], fb, abs_x, abs_y);
    }
}
