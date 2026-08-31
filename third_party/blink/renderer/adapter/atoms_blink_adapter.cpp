/*
 * ATOMS OS — Google Chromium / Blink Rendering Engine Adapter Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "atoms_blink_adapter.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/layout/layout_tree_builder.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/paint/blink_skia_painter.h"
#include "third_party/skia/include/adapter/atoms_skia_adapter.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"

extern "C" {

bool AtomsBlink_RenderHTML(const char* html_source, void* bwe_window_ptr, int viewport_w, int viewport_h) {
    if (!html_source || !bwe_window_ptr || viewport_w <= 0 || viewport_h <= 0) return false;

    // 1. Instantiate Blink Document
    blink::Document document;

    // 2. Parse HTML & execute any embedded <script> tags
    document.parseHTML(html_source);

    // 3. Construct Blink Layout Tree
    blink::LayoutObject* root_layout = blink::LayoutTreeBuilder::buildLayoutTree(&document);
    if (!root_layout) return false;

    // 4. Calculate layout box geometry
    root_layout->layout(0, 0, viewport_w);

    // 5. Wrap BWE window in AtomsSkiaSurface
    AtomsSkiaSurface skia_surface((uint32_t*)bwe_window_ptr, viewport_w, viewport_h);

    SkCanvas* canvas = skia_surface.GetCanvas();
    if (canvas) {
        // Clear background with document base color
        canvas->clear(0xFF181825);

        // 6. Paint Blink Layout tree to Skia canvas
        blink::BlinkSkiaPainter::paint(root_layout, canvas);

        // 7. Flush memory and invalidate window
        skia_surface.Flush();
    }

    delete root_layout;
    return true;
}


} // extern "C"
