/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "layout_tree_builder.h"
#include "layout_block.h"
#include "layout_inline.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"

namespace blink {

LayoutObject* LayoutTreeBuilder::createLayoutObject(Node* node) {
    if (!node) return nullptr;

    if (node->isElementNode()) {
        Element* el = static_cast<Element*>(node);
        if (el->style()->isHidden()) return nullptr;
        if (el->getTagName() == "head" || el->getTagName() == "title" ||
            el->getTagName() == "style" || el->getTagName() == "script") {
            return nullptr;
        }
        return new LayoutBlock(node);
    } else if (node->isTextNode()) {
        return new LayoutInline(node);
    }
    return nullptr;
}

static void BuildChildren(Node* dom_node, LayoutObject* layout_parent) {
    if (!dom_node || !layout_parent) return;

    for (Node* cur = dom_node->getFirstChild(); cur; cur = cur->getNextSibling()) {
        LayoutObject* child_layout = LayoutTreeBuilder::createLayoutObject(cur);
        if (child_layout) {
            layout_parent->appendChild(child_layout);
            BuildChildren(cur, child_layout);
        }
    }
}

LayoutObject* LayoutTreeBuilder::buildLayoutTree(Document* document) {
    if (!document) return nullptr;
    Element* body = document->getBody();
    if (!body) body = document->getDocumentElement();
    if (!body) return nullptr;

    LayoutObject* root = createLayoutObject(body);
    if (root) {
        BuildChildren(body, root);
    }
    return root;
}

} // namespace blink
