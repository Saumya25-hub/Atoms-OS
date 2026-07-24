#include "abe_flex_layout.h"
#include "abe_box_model.h"
#include "../diagnostics/abe_diagnostics.h"

static bool g_flex_layout_initialized = false;

ABE_Error ABE_FlexLayout_Init(void) {
    g_flex_layout_initialized = true;
    ABE_Log(ABE_LOG_INFO, "FLEXLAYOUT", "ABE Flexbox Layout Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_FlexLayout_Shutdown(void) {
    g_flex_layout_initialized = false;
    return ABE_SUCCESS;
}

void ABE_FlexLayout_Perform(ABE_RenderNode* flex_node, float containing_width, float containing_height) {
    if (!flex_node) return;

    ABE_BoxModel_Compute(flex_node, containing_width, containing_height);

    // Flex direction row (horizontal alignment)
    float cur_x = flex_node->content_box.x + flex_node->padding.left;
    float cur_y = flex_node->content_box.y + flex_node->padding.top;
    float max_child_h = 0.0f;

    ABE_RenderNode* child = flex_node->first_child;
    while (child) {
        ABE_BoxModel_Compute(child, flex_node->content_box.width, containing_height);

        child->content_box.x = cur_x + child->margin.left + child->border.left;
        child->content_box.y = cur_y + child->margin.top + child->border.top;

        float child_w = child->content_box.width + child->margin.left + child->margin.right + child->padding.left + child->padding.right;
        float child_h = child->content_box.height + child->margin.top + child->margin.bottom + child->padding.top + child->padding.bottom;

        if (child_h > max_child_h) max_child_h = child_h;
        cur_x += child_w;

        child = child->next_sibling;
    }

    if (flex_node->style.height_auto) {
        flex_node->content_box.height = (max_child_h > 0.0f) ? max_child_h : 40.0f;
    }
}
