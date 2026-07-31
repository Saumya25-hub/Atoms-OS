#include "framework/include/bos_ui_core.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"

void BOS_UIElement_Init(BOS_UIElement* elem, const char* type_name) {
    if (!elem) return;
    memset(elem, 0, sizeof(BOS_UIElement));
    
    static uint32_t id_counter = 1;
    elem->id = id_counter++;
    elem->type_name = type_name ? type_name : "UIElement";
    elem->visible = true;
    elem->enabled = true;
    elem->opacity = 1.0f;
    elem->is_layout_valid = false;
}

void BOS_UIElement_AddChild(BOS_UIElement* parent, BOS_UIElement* child) {
    if (!parent || !child) return;

    if (parent->child_count >= parent->child_capacity) {
        uint32_t new_cap = (parent->child_capacity == 0) ? 8 : (parent->child_capacity * 2);
        BOS_UIElement** new_array = (BOS_UIElement**)kmalloc(sizeof(BOS_UIElement*) * new_cap);
        if (!new_array) return;

        if (parent->children && parent->child_count > 0) {
            memcpy(new_array, parent->children, sizeof(BOS_UIElement*) * parent->child_count);
            kfree(parent->children);
        }
        parent->children = new_array;
        parent->child_capacity = new_cap;
    }

    parent->children[parent->child_count++] = child;
    child->parent = parent;
    BOS_UIElement_InvalidateLayout(parent);
}

void BOS_UIElement_RemoveChild(BOS_UIElement* parent, BOS_UIElement* child) {
    if (!parent || !child || parent->child_count == 0) return;

    for (uint32_t i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            child->parent = NULL;
            for (uint32_t j = i; j < parent->child_count - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->child_count--;
            BOS_UIElement_InvalidateLayout(parent);
            break;
        }
    }
}

void BOS_UIElement_InvalidateLayout(BOS_UIElement* elem) {
    while (elem) {
        elem->is_layout_valid = false;
        elem = elem->parent;
    }
}
