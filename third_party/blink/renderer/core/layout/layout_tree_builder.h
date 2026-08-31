/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_TREE_BUILDER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_TREE_BUILDER_H_

namespace blink {

class Document;
class Node;
class LayoutObject;

class LayoutTreeBuilder {
public:
    static LayoutObject* buildLayoutTree(Document* document);
    static LayoutObject* createLayoutObject(Node* node);
};


} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_TREE_BUILDER_H_
