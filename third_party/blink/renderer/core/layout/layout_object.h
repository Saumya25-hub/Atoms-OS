/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_OBJECT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_OBJECT_H_

#include <stddef.h>
#include <stdint.h>
#include "userspace/runtime/cpp/include/vector"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/css/css_style_declaration.h"

namespace blink {

class LayoutBlock;

class LayoutObject {
public:
    LayoutObject(Node* node);
    virtual ~LayoutObject();

    Node* getNode() const { return node_; }
    LayoutObject* getParent() const { return parent_; }
    void setParent(LayoutObject* p) { parent_ = p; }

    LayoutObject* getFirstChild() const { return first_child_; }
    LayoutObject* getNextSibling() const { return next_sibling_; }

    void appendChild(LayoutObject* child);

    virtual void layout(int container_x, int container_y, int container_width) = 0;

    int getX() const { return x_; }
    int getY() const { return y_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    virtual bool isLayoutBlock() const { return false; }
    virtual bool isLayoutInline() const { return false; }

    uint32_t getColor() const;
    uint32_t getBackgroundColor() const;
    int getFontSize() const;

protected:
    Node* node_;
    LayoutObject* parent_;
    LayoutObject* first_child_;
    LayoutObject* last_child_;
    LayoutObject* next_sibling_;

    int x_;
    int y_;
    int width_;
    int height_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_OBJECT_H_
