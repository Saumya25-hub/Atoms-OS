/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "layout_object.h"
#include "third_party/blink/renderer/core/dom/element.h"

namespace blink {

LayoutObject::LayoutObject(Node* node)
    : node_(node)
    , parent_(nullptr)
    , first_child_(nullptr)
    , last_child_(nullptr)
    , next_sibling_(nullptr)
    , x_(0)
    , y_(0)
    , width_(0)
    , height_(0)
{
    if (node_) {
        node_->setLayoutObject(this);
    }
}

LayoutObject::~LayoutObject() {
    LayoutObject* cur = first_child_;
    while (cur) {
        LayoutObject* next = cur->getNextSibling();
        delete cur;
        cur = next;
    }
    if (node_) {
        node_->setLayoutObject(nullptr);
    }
}

void LayoutObject::appendChild(LayoutObject* child) {
    if (!child) return;
    child->setParent(this);
    if (last_child_) {
        last_child_->next_sibling_ = child;
    } else {
        first_child_ = child;
    }
    last_child_ = child;
}

uint32_t LayoutObject::getColor() const {
    if (node_ && node_->isElementNode()) {
        Element* el = static_cast<Element*>(node_);
        return el->style()->getColor(0xFFCDD6F4);
    }
    if (parent_) return parent_->getColor();
    return 0xFFCDD6F4;
}

uint32_t LayoutObject::getBackgroundColor() const {
    if (node_ && node_->isElementNode()) {
        Element* el = static_cast<Element*>(node_);
        return el->style()->getBackgroundColor(0x00000000);
    }
    return 0x00000000;
}

int LayoutObject::getFontSize() const {
    if (node_ && node_->isElementNode()) {
        Element* el = static_cast<Element*>(node_);
        return el->style()->getFontSize(14);
    }
    if (parent_) return parent_->getFontSize();
    return 14;
}

} // namespace blink
