#include "../Include/layout.h"

BVLayoutResult BV_CalculateLayout(BVRect bounds, BVPadding padding, BVTextMetrics textMetrics, BVLayoutAlignment hAlign, BVLayoutAlignment vAlign) {
    BVLayoutResult result;

    // 1. Calculate Content Bounds (Bounds minus Padding)
    result.content_bounds = BV_RectDeflate(bounds, padding);

    // 2. Calculate Text Bounds within Content Bounds
    result.text_bounds.width = textMetrics.width;
    result.text_bounds.height = textMetrics.height;

    // Horizontal Alignment
    if (hAlign == BV_ALIGN_START) {
        result.text_bounds.x = result.content_bounds.x;
    } else if (hAlign == BV_ALIGN_CENTER) {
        result.text_bounds.x = result.content_bounds.x + (result.content_bounds.width - textMetrics.width) / 2;
    } else if (hAlign == BV_ALIGN_END) {
        result.text_bounds.x = result.content_bounds.x + result.content_bounds.width - textMetrics.width;
    }

    // Vertical Alignment
    if (vAlign == BV_ALIGN_START) {
        result.text_bounds.y = result.content_bounds.y;
    } else if (vAlign == BV_ALIGN_CENTER) {
        result.text_bounds.y = result.content_bounds.y + (result.content_bounds.height - textMetrics.height) / 2;
    } else if (vAlign == BV_ALIGN_END) {
        result.text_bounds.y = result.content_bounds.y + result.content_bounds.height - textMetrics.height;
    }

    return result;
}
