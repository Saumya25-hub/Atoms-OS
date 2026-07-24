#include "abe_box_model.h"
#include "../diagnostics/abe_diagnostics.h"

static bool g_box_model_initialized = false;

ABE_Error ABE_BoxModel_Init(void) {
    g_box_model_initialized = true;
    ABE_Log(ABE_LOG_INFO, "BOXMODEL", "ABE CSS Box Model Subsystem initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_BoxModel_Shutdown(void) {
    g_box_model_initialized = false;
    return ABE_SUCCESS;
}

float ABE_BoxModel_CollapseMargins(float margin_a, float margin_b) {
    if (margin_a >= margin_b) return margin_a;
    return margin_b;
}

void ABE_BoxModel_Compute(ABE_RenderNode* node, float containing_width, float containing_height) {
    if (!node) return;

    // 1. Margins
    node->margin.top = node->style.margin_top_px;
    node->margin.right = node->style.margin_right_px;
    node->margin.bottom = node->style.margin_bottom_px;
    node->margin.left = node->style.margin_left_px;

    // 2. Padding
    node->padding.top = node->style.padding_top_px;
    node->padding.right = node->style.padding_right_px;
    node->padding.bottom = node->style.padding_bottom_px;
    node->padding.left = node->style.padding_left_px;

    // 3. Border Width
    node->border.top = node->style.border_top_width_px;
    node->border.right = node->style.border_right_width_px;
    node->border.bottom = node->style.border_bottom_width_px;
    node->border.left = node->style.border_left_width_px;

    // 4. Content Width
    if (node->style.width_auto) {
        float available_w = containing_width - (node->margin.left + node->margin.right +
                                                node->padding.left + node->padding.right +
                                                node->border.left + node->border.right);
        node->content_box.width = (available_w > 0.0f) ? available_w : 0.0f;
    } else {
        node->content_box.width = node->style.width_px;
    }

    // 5. Content Height (Initial calculation, expanded by children in Block Layout)
    if (!node->style.height_auto) {
        node->content_box.height = node->style.height_px;
    } else {
        node->content_box.height = 0.0f;
    }
}
