/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "blink_skia_painter.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkColor.h"

namespace blink {

void BlinkSkiaPainter::paint(LayoutObject* root_layout, void* sk_canvas) {
    if (!root_layout || !sk_canvas) return;
    SkCanvas* canvas = static_cast<SkCanvas*>(sk_canvas);
    paintObject(root_layout, canvas);
}

void BlinkSkiaPainter::paintObject(LayoutObject* obj, void* sk_canvas) {
    if (!obj || !sk_canvas) return;
    SkCanvas* canvas = static_cast<SkCanvas*>(sk_canvas);

    // 1. Paint Background if specified
    uint32_t bg_color = obj->getBackgroundColor();
    if ((bg_color & 0xFF000000) != 0) {
        SkPaint bg_paint;
        bg_paint.setColor(bg_color);
        bg_paint.setStyle(SkPaint::kFill_Style);
        SkRect rect = SkRect::MakeXYWH((float)obj->getX(), (float)obj->getY(),
                                       (float)obj->getWidth(), (float)obj->getHeight());
        canvas->drawRect(rect, bg_paint);
    }

    // 2. Paint Text / Foreground if inline
    if (obj->isLayoutInline()) {
        uint32_t text_color = obj->getColor();
        SkPaint text_paint;
        text_paint.setColor(text_color);
        text_paint.setStyle(SkPaint::kFill_Style);
        SkRect text_rect = SkRect::MakeXYWH((float)obj->getX(), (float)obj->getY() + 2.0f,
                                            (float)obj->getWidth(), (float)obj->getHeight() - 4.0f);
        canvas->drawRect(text_rect, text_paint);
    }

    // 3. Recursively paint child layout objects
    for (LayoutObject* cur = obj->getFirstChild(); cur; cur = cur->getNextSibling()) {
        paintObject(cur, canvas);
    }
}

} // namespace blink
