/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "container_node.h"
#include "text.h"

namespace blink {

ContainerNode::ContainerNode(Document* document, NodeType type)
    : Node(document, type)
    , first_child_(nullptr)
    , last_child_(nullptr)
{
}

ContainerNode::~ContainerNode() {
    Node* cur = first_child_;
    while (cur) {
        Node* next = cur->getNextSibling();
        delete cur;
        cur = next;
    }
}

Node* ContainerNode::appendChild(Node* child) {
    if (!child) return nullptr;

    if (child->getParentNode()) {
        child->getParentNode()->removeChild(child);
    }

    child->setParentNode(this);
    child->setPreviousSibling(last_child_);
    child->setNextSibling(nullptr);

    if (last_child_) {
        last_child_->setNextSibling(child);
    } else {
        first_child_ = child;
    }
    last_child_ = child;

    return child;
}

Node* ContainerNode::removeChild(Node* child) {
    if (!child || child->getParentNode() != this) return nullptr;

    if (child->getPreviousSibling()) {
        child->getPreviousSibling()->setNextSibling(child->getNextSibling());
    } else {
        first_child_ = child->getNextSibling();
    }

    if (child->getNextSibling()) {
        child->getNextSibling()->setPreviousSibling(child->getPreviousSibling());
    } else {
        last_child_ = child->getPreviousSibling();
    }

    child->setParentNode(nullptr);
    child->setPreviousSibling(nullptr);
    child->setNextSibling(nullptr);

    return child;
}

Node* ContainerNode::insertBefore(Node* new_child, Node* ref_child) {
    if (!new_child) return nullptr;
    if (!ref_child) return appendChild(new_child);
    if (ref_child->getParentNode() != this) return nullptr;

    if (new_child->getParentNode()) {
        new_child->getParentNode()->removeChild(new_child);
    }

    new_child->setParentNode(this);
    new_child->setPreviousSibling(ref_child->getPreviousSibling());
    new_child->setNextSibling(ref_child);

    if (ref_child->getPreviousSibling()) {
        ref_child->getPreviousSibling()->setNextSibling(new_child);
    } else {
        first_child_ = new_child;
    }
    ref_child->setPreviousSibling(new_child);

    return new_child;
}

std::string ContainerNode::getTextContent() const {
    std::string res = "";
    Node* cur = first_child_;
    while (cur) {
        res += cur->getTextContent();
        cur = cur->getNextSibling();
    }
    return res;
}

void ContainerNode::setTextContent(const std::string& text) {
    // Remove all existing children
    while (first_child_) {
        Node* c = removeChild(first_child_);
        delete c;
    }
    if (!text.empty()) {
        appendChild(new Text(getOwnerDocument(), text));
    }
}

size_t ContainerNode::getChildCount() const {
    size_t count = 0;
    Node* cur = first_child_;
    while (cur) {
        count++;
        cur = cur->getNextSibling();
    }
    return count;
}

Node* ContainerNode::getChildAt(size_t index) const {
    size_t i = 0;
    Node* cur = first_child_;
    while (cur) {
        if (i == index) return cur;
        i++;
        cur = cur->getNextSibling();
    }
    return nullptr;
}

} // namespace blink
