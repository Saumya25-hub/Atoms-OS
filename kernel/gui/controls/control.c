#include "kernel/gui/controls/control.h"
#include "kernel/gui/window/window.h"
#include "kernel/core/memory.h" // Assuming kmalloc/kfree exists, or similar
#include <stddef.h>

void control_init(BOSControl* control) {
    if (!control) return;
    
    control->x = 0;
    control->y = 0;
    control->width = 100;
    control->height = 24;
    
    control->visible = true;
    control->enabled = true;
    control->focused = false;
    control->hovered = false;
    control->pressed = false;
    
    control->id = 0;
    control->window = NULL;
    
    control->parent = NULL;
    control->first_child = NULL;
    control->next_sibling = NULL;
    
    control->paint = NULL;
    control->handle_event = NULL;
    control->destroy = NULL;
    control->priv_data = NULL;
}

void control_set_position(BOSControl* control, int x, int y) {
    if (!control) return;
    control_invalidate(control); // Invalidate old pos
    control->x = x;
    control->y = y;
    control_invalidate(control); // Invalidate new pos
}

void control_set_size(BOSControl* control, int width, int height) {
    if (!control) return;
    control_invalidate(control); // Invalidate old size
    control->width = width;
    control->height = height;
    control_invalidate(control); // Invalidate new size
}

void control_set_visible(BOSControl* control, bool visible) {
    if (!control) return;
    if (control->visible != visible) {
        control->visible = visible;
        control_invalidate(control);
    }
}

void control_set_enabled(BOSControl* control, bool enabled) {
    if (!control) return;
    if (control->enabled != enabled) {
        control->enabled = enabled;
        control_invalidate(control);
    }
}

void control_add_child(BOSControl* parent, BOSControl* child) {
    if (!parent || !child) return;
    
    child->parent = parent;
    child->window = parent->window;
    
    if (!parent->first_child) {
        parent->first_child = child;
    } else {
        BOSControl* curr = parent->first_child;
        while (curr->next_sibling) {
            curr = curr->next_sibling;
        }
        curr->next_sibling = child;
    }
    control_invalidate(child);
}

void control_remove_child(BOSControl* parent, BOSControl* child) {
    if (!parent || !child || !parent->first_child) return;
    
    control_invalidate(child);
    
    if (parent->first_child == child) {
        parent->first_child = child->next_sibling;
    } else {
        BOSControl* curr = parent->first_child;
        while (curr && curr->next_sibling != child) {
            curr = curr->next_sibling;
        }
        if (curr) {
            curr->next_sibling = child->next_sibling;
        }
    }
    child->parent = NULL;
    child->next_sibling = NULL;
}

void control_get_absolute_position(BOSControl* control, int* abs_x, int* abs_y) {
    int ax = 0;
    int ay = 0;
    BOSControl* curr = control;
    while (curr) {
        ax += curr->x;
        ay += curr->y;
        curr = curr->parent;
    }
    
    // Add window client area offset if needed here, but for now we assume 
    // window surface 0,0 maps to client area 0,0.
    
    if (abs_x) *abs_x = ax;
    if (abs_y) *abs_y = ay;
}

void control_invalidate(BOSControl* control) {
    if (!control || !control->window || !control->visible) return;
    
    int ax, ay;
    control_get_absolute_position(control, &ax, &ay);
    
    // Send invalidate to window manager for this rect
    // We will implement window_invalidate_rect in window.c later
    extern void window_invalidate_rect(struct BOSWindow* window, int x, int y, int width, int height);
    window_invalidate_rect(control->window, ax, ay, control->width, control->height);
}

void control_paint_children(BOSControl* parent, struct BOSSurface* surface, const BVRect* clip) {
    if (!parent || !parent->first_child) return;
    
    BOSControl* curr = parent->first_child;
    while (curr) {
        if (curr->visible && curr->paint) {
            curr->paint(curr, surface, clip);
        }
        curr = curr->next_sibling;
    }
}

BOSControl* control_hit_test(BOSControl* root, int x, int y) {
    if (!root || !root->visible) return NULL;
    
    // Check if point is inside this control
    if (x >= root->x && x < root->x + root->width &&
        y >= root->y && y < root->y + root->height) {
        
        // Convert coords to child local space
        int child_x = x - root->x;
        int child_y = y - root->y;
        
        // Reverse order check for Z-order (top-most first)
        // Since it's a singly linked list, we'll just check forward for now.
        // A real impl might build an array or doubly-linked list.
        BOSControl* hit = NULL;
        BOSControl* curr = root->first_child;
        while (curr) {
            BOSControl* child_hit = control_hit_test(curr, child_x, child_y);
            if (child_hit) {
                hit = child_hit;
            }
            curr = curr->next_sibling;
        }
        
        return hit ? hit : root;
    }
    
    return NULL;
}

void control_destroy_recursive(BOSControl* control) {
    if (!control) return;
    
    BOSControl* curr = control->first_child;
    while (curr) {
        BOSControl* next = curr->next_sibling;
        control_destroy_recursive(curr);
        curr = next;
    }
    
    if (control->destroy) {
        control->destroy(control);
    }
}
