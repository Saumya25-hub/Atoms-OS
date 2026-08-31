/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "layout_inline.h"
#include "third_party/blink/renderer/core/dom/text.h"

namespace blink {

LayoutInline::LayoutInline(Node* node)
    : LayoutObject(node)
{
}

void LayoutInline::layout(int container_x, int container_y, int container_width) {
    x_ = container_x;
    y_ = container_y;
    width_ = container_width;

    int text_len = 0;
    if (node_ && node_->isTextNode()) {
        Text* t = static_cast<Text*>(node_);
        text_len = (int)t->length();
    }

    int font_sz = getFontSize();
    height_ = font_sz + 4;
    width_ = text_len * (font_sz / 2 + 1);
    if (width_ > container_width && container_width > 0) {
        width_ = container_width;
    }
}

} // namespace blink
