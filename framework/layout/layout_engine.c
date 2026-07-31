#include "framework/include/bos_ui_layout.h"

void BOS_Layout_MeasurePass(BOS_UIElement* elem, BOS_Size available_size) {
    if (!elem || !elem->visible) return;

    /* Subtract margins from available size */
    int32_t net_w = (int32_t)available_size.width - (elem->margin.left + elem->margin.right);
    int32_t net_h = (int32_t)available_size.height - (elem->margin.top + elem->margin.bottom);
    if (net_w < 0) net_w = 0;
    if (net_h < 0) net_h = 0;

    BOS_Size child_available = { (uint32_t)net_w, (uint32_t)net_h };

    /* Virtual Measure Hook */
    if (elem->measure) {
        elem->measure(elem, child_available);
    } else {
        /* Default Measure Behavior: Accumulate children sizes */
        BOS_Size max_desired = { 0, 0 };
        for (uint32_t i = 0; i < elem->child_count; i++) {
            if (elem->children[i] && elem->children[i]->visible) {
                BOS_Layout_MeasurePass(elem->children[i], child_available);
                if (elem->children[i]->desired_size.width > max_desired.width) {
                    max_desired.width = elem->children[i]->desired_size.width;
                }
                if (elem->children[i]->desired_size.height > max_desired.height) {
                    max_desired.height = elem->children[i]->desired_size.height;
                }
            }
        }
        elem->desired_size = max_desired;
    }

    /* Enforce Min/Max Size Constraints */
    if (elem->min_size.width > 0 && elem->desired_size.width < elem->min_size.width) {
        elem->desired_size.width = elem->min_size.width;
    }
    if (elem->min_size.height > 0 && elem->desired_size.height < elem->min_size.height) {
        elem->desired_size.height = elem->min_size.height;
    }
}

void BOS_Layout_ArrangePass(BOS_UIElement* elem, BOS_Rect final_rect) {
    if (!elem || !elem->visible) return;

    /* Apply Margin Offsets */
    final_rect.x += elem->margin.left;
    final_rect.y += elem->margin.top;
    final_rect.width -= (elem->margin.left + elem->margin.right);
    final_rect.height -= (elem->margin.top + elem->margin.bottom);

    elem->bounds = final_rect;

    /* Virtual Arrange Hook */
    if (elem->arrange) {
        elem->arrange(elem, final_rect);
    } else {
        /* Default Arrange Behavior: Give full client rect to children */
        for (uint32_t i = 0; i < elem->child_count; i++) {
            if (elem->children[i] && elem->children[i]->visible) {
                BOS_Layout_ArrangePass(elem->children[i], final_rect);
            }
        }
    }

    elem->is_layout_valid = true;
}
