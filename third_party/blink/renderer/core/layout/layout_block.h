/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_BLOCK_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_BLOCK_H_

#include "layout_object.h"

namespace blink {

class LayoutBlock : public LayoutObject {
public:
    explicit LayoutBlock(Node* node);
    ~LayoutBlock() override = default;

    void layout(int container_x, int container_y, int container_width) override;
    bool isLayoutBlock() const override { return true; }
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_BLOCK_H_
