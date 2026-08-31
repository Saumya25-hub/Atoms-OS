/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "node.h"
#include "container_node.h"

namespace blink {

Node::Node(Document* document, NodeType type)
    : document_(document)
    , type_(type)
    , parent_node_(nullptr)
    , previous_sibling_(nullptr)
    , next_sibling_(nullptr)
    , layout_object_(nullptr)
{
}

Node::~Node() {
}

Node* Node::appendChild(Node* child) {
    (void)child;
    return nullptr;
}

Node* Node::removeChild(Node* child) {
    (void)child;
    return nullptr;
}

Node* Node::insertBefore(Node* new_child, Node* ref_child) {
    (void)new_child;
    (void)ref_child;
    return nullptr;
}

std::string Node::getTextContent() const {
    return "";
}

void Node::setTextContent(const std::string& text) {
    (void)text;
}

} // namespace blink
