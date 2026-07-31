#include "framework/include/bos_ui_layout.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static void stackpanel_measure(BOS_UIElement* self, BOS_Size available_size) {
    BOS_StackPanel* panel = (BOS_StackPanel*)self;
    BOS_Size desired = { 0, 0 };

    for (uint32_t i = 0; i < self->child_count; i++) {
        BOS_UIElement* child = self->children[i];
        if (!child || !child->visible) continue;

        BOS_Layout_MeasurePass(child, available_size);

        if (panel->orientation == BOS_ORIENTATION_VERTICAL) {
            desired.height += child->desired_size.height + panel->spacing;
            if (child->desired_size.width > desired.width) {
                desired.width = child->desired_size.width;
            }
        } else {
            desired.width += child->desired_size.width + panel->spacing;
            if (child->desired_size.height > desired.height) {
                desired.height = child->desired_size.height;
            }
        }
    }

    self->desired_size = desired;
}

static void stackpanel_arrange(BOS_UIElement* self, BOS_Rect final_rect) {
    BOS_StackPanel* panel = (BOS_StackPanel*)self;
    int32_t current_pos = 0;

    for (uint32_t i = 0; i < self->child_count; i++) {
        BOS_UIElement* child = self->children[i];
        if (!child || !child->visible) continue;

        BOS_Rect child_rect;
        if (panel->orientation == BOS_ORIENTATION_VERTICAL) {
            child_rect.x = final_rect.x;
            child_rect.y = final_rect.y + current_pos;
            child_rect.width = final_rect.width;
            child_rect.height = child->desired_size.height;
            current_pos += (int32_t)child->desired_size.height + panel->spacing;
        } else {
            child_rect.x = final_rect.x + current_pos;
            child_rect.y = final_rect.y;
            child_rect.width = child->desired_size.width;
            child_rect.height = final_rect.height;
            current_pos += (int32_t)child->desired_size.width + panel->spacing;
        }

        BOS_Layout_ArrangePass(child, child_rect);
    }
}

BOS_StackPanel* BOS_StackPanel_Create(BOS_Orientation orientation) {
    BOS_StackPanel* panel = (BOS_StackPanel*)kmalloc(sizeof(BOS_StackPanel));
    if (!panel) return NULL;

    memset(panel, 0, sizeof(BOS_StackPanel));
    BOS_UIElement_Init(&panel->base, "StackPanel");
    panel->orientation = orientation;
    panel->spacing = 5;
    panel->base.measure = stackpanel_measure;
    panel->base.arrange = stackpanel_arrange;
    return panel;
}
