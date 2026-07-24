#include "abe_block_layout.h"
#include "abe_box_model.h"
#include "abe_inline_layout.h"
#include "abe_flex_layout.h"
#include "../diagnostics/abe_diagnostics.h"

static bool g_block_layout_initialized = false;

ABE_Error ABE_BlockLayout_Init(void) {
    g_block_layout_initialized = true;
    ABE_Log(ABE_LOG_INFO, "BLOCKLAYOUT", "ABE Block Layout Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_BlockLayout_Shutdown(void) {
    g_block_layout_initialized = false;
    return ABE_SUCCESS;
}

void ABE_BlockLayout_Perform(ABE_RenderNode* block_node, float containing_width, float containing_height) {
    if (!block_node) return;

    // 1. Compute Box Model for current block node
    ABE_BoxModel_Compute(block_node, containing_width, containing_height);

    float current_y = block_node->content_box.y + block_node->padding.top;
    float current_x = block_node->content_box.x + block_node->padding.left;
    float previous_bottom_margin = 0.0f;

    ABE_RenderNode* child = block_node->first_child;
    while (child) {
        if (child->is_flex) {
            // Delegate to Flexbox Engine
            ABE_FlexLayout_Perform(child, block_node->content_box.width, containing_height);
            child->content_box.x = current_x + child->margin.left + child->border.left;
            child->content_box.y = current_y + child->margin.top + child->border.top;

            float child_total_h = child->margin.top + child->border.top + child->padding.top +
                                  child->content_box.height + child->padding.bottom + child->border.bottom + child->margin.bottom;
            current_y += child_total_h;
        } else if (child->is_inline) {
            // Delegate to Inline Engine
            ABE_InlineLayout_Perform(child, current_x, current_y, block_node->content_box.width);
            float inline_h = child->margin.top + child->border.top + child->padding.top +
                             child->content_box.height + child->padding.bottom + child->border.bottom + child->margin.bottom;
            current_y += inline_h;
        } else {
            // Recursive Block Layout
            child->content_box.x = current_x + child->margin.left + child->border.left;

            // Margin Collapsing
            float collapsed_margin = ABE_BoxModel_CollapseMargins(previous_bottom_margin, child->margin.top);
            current_y += collapsed_margin;
            child->content_box.y = current_y + child->border.top;

            ABE_BlockLayout_Perform(child, block_node->content_box.width, containing_height);

            float child_outer_h = child->border.top + child->padding.top + child->content_box.height +
                                  child->padding.bottom + child->border.bottom;
            current_y += child_outer_h;
            previous_bottom_margin = child->margin.bottom;
        }

        child->is_dirty = false;
        child = child->next_sibling;
    }

    // If height is auto, aggregate from content children
    if (block_node->style.height_auto) {
        float computed_h = current_y - (block_node->content_box.y + block_node->padding.top);
        block_node->content_box.height = (computed_h > 0.0f) ? computed_h : 20.0f;
    }

    // Overflow Box = Content Box + Margins/Paddings/Borders
    block_node->overflow_box.x = block_node->content_box.x - block_node->margin.left;
    block_node->overflow_box.y = block_node->content_box.y - block_node->margin.top;
    block_node->overflow_box.width = block_node->content_box.width + block_node->margin.left + block_node->margin.right + block_node->padding.left + block_node->padding.right;
    block_node->overflow_box.height = block_node->content_box.height + block_node->margin.top + block_node->margin.bottom + block_node->padding.top + block_node->padding.bottom;
}
