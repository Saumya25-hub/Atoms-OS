/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "layout_block.h"
#include "third_party/blink/renderer/core/dom/element.h"

namespace blink {

LayoutBlock::LayoutBlock(Node* node)
    : LayoutObject(node)
{
}

void LayoutBlock::layout(int container_x, int container_y, int container_width) {
    x_ = container_x;
    y_ = container_y;
    width_ = container_width;

    int margin = 0;
    int padding = 0;

    if (node_ && node_->isElementNode()) {
        Element* el = static_cast<Element*>(node_);
        margin = el->style()->getMargin(0);
        padding = el->style()->getPadding(0);
        int w = el->style()->getWidth(-1);
        if (w > 0) width_ = w;
    }

    int current_y = y_ + margin + padding;
    int child_x = x_ + margin + padding;
    int child_available_width = width_ - (margin * 2) - (padding * 2);
    if (child_available_width < 0) child_available_width = 0;

    LayoutObject* child = first_child_;
    while (child) {
        child->layout(child_x, current_y, child_available_width);
        current_y += child->getHeight();
        child = child->getNextSibling();
    }

    height_ = (current_y - y_) + margin + padding;
    if (node_ && node_->isElementNode()) {
        Element* el = static_cast<Element*>(node_);
        int h = el->style()->getHeight(-1);
        if (h > 0) height_ = h;
        if (el->getTagName() == "h1" && height_ < 36) height_ = 36;
        if (el->getTagName() == "p" && height_ < 24) height_ = 24;
    }
}

} // namespace blink
