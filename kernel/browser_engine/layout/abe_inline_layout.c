#include "abe_inline_layout.h"
#include "abe_box_model.h"
#include "../diagnostics/abe_diagnostics.h"

static bool g_inline_layout_initialized = false;

ABE_Error ABE_InlineLayout_Init(void) {
    g_inline_layout_initialized = true;
    ABE_Log(ABE_LOG_INFO, "INLINELAYOUT", "ABE Inline Layout Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_InlineLayout_Shutdown(void) {
    g_inline_layout_initialized = false;
    return ABE_SUCCESS;
}

void ABE_InlineLayout_Perform(ABE_RenderNode* inline_node, float start_x, float start_y, float max_width) {
    if (!inline_node) return;

    ABE_BoxModel_Compute(inline_node, max_width, 0.0f);

    inline_node->content_box.x = start_x + inline_node->margin.left + inline_node->border.left;
    inline_node->content_box.y = start_y + inline_node->margin.top + inline_node->border.top;

    float current_x = inline_node->content_box.x;
    float current_y = inline_node->content_box.y;
    float line_height = inline_node->style.line_height_px > 0.0f ? inline_node->style.line_height_px : 20.0f;

    ABE_RenderNode* child = inline_node->first_child;
    while (child) {
        float child_w = child->style.width_auto ? 60.0f : child->style.width_px;

        // Line wrap check
        if (current_x + child_w > start_x + max_width && current_x > start_x) {
            current_x = start_x;
            current_y += line_height;
        }

        child->content_box.x = current_x;
        child->content_box.y = current_y;
        child->content_box.width = child_w;
        child->content_box.height = line_height;

        current_x += child_w + 5.0f; // 5px spacing between inline items
        child = child->next_sibling;
    }

    if (inline_node->style.width_auto) {
        inline_node->content_box.width = (current_x - inline_node->content_box.x);
    }
    if (inline_node->style.height_auto) {
        inline_node->content_box.height = (current_y + line_height - inline_node->content_box.y);
    }
}
