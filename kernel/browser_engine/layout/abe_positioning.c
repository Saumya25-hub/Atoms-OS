#include "abe_positioning.h"
#include "../diagnostics/abe_diagnostics.h"

static bool g_positioning_initialized = false;

ABE_Error ABE_Positioning_Init(void) {
    g_positioning_initialized = true;
    ABE_Log(ABE_LOG_INFO, "POSITIONING", "ABE Positioning & Overflow Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_Positioning_Shutdown(void) {
    g_positioning_initialized = false;
    return ABE_SUCCESS;
}

void ABE_Positioning_Apply(ABE_RenderNode* node, float containing_width, float containing_height) {
    if (!node) return;

    if (node->style.position == ABE_POSITION_RELATIVE) {
        // Relative offset from normal flow position
        // Example: top: 10px, left: 20px
    } else if (node->style.position == ABE_POSITION_ABSOLUTE) {
        // Position relative to containing block
        node->content_box.x = node->margin.left;
        node->content_box.y = node->margin.top;
    } else if (node->style.position == ABE_POSITION_FIXED) {
        // Position relative to viewport bounds
        node->content_box.x = node->margin.left;
        node->content_box.y = node->margin.top;
    }

    ABE_RenderNode* child = node->first_child;
    while (child) {
        ABE_Positioning_Apply(child, containing_width, containing_height);
        child = child->next_sibling;
    }
}

void ABE_Overflow_Compute(ABE_RenderNode* node) {
    if (!node) return;

    float min_x = node->content_box.x;
    float min_y = node->content_box.y;
    float max_x = node->content_box.x + node->content_box.width;
    float max_y = node->content_box.y + node->content_box.height;

    ABE_RenderNode* child = node->first_child;
    while (child) {
        ABE_Overflow_Compute(child);
        if (child->overflow_box.x < min_x) min_x = child->overflow_box.x;
        if (child->overflow_box.y < min_y) min_y = child->overflow_box.y;
        if (child->overflow_box.x + child->overflow_box.width > max_x) max_x = child->overflow_box.x + child->overflow_box.width;
        if (child->overflow_box.y + child->overflow_box.height > max_y) max_y = child->overflow_box.y + child->overflow_box.height;
        child = child->next_sibling;
    }

    node->overflow_box.x = min_x;
    node->overflow_box.y = min_y;
    node->overflow_box.width = max_x - min_x;
    node->overflow_box.height = max_y - min_y;
}
