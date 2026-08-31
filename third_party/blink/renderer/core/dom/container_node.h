/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_CONTAINER_NODE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_CONTAINER_NODE_H_

#include "node.h"

namespace blink {

class ContainerNode : public Node {
public:
    ContainerNode(Document* document, NodeType type);
    ~ContainerNode() override;

    Node* getFirstChild() const override { return first_child_; }
    Node* getLastChild() const override { return last_child_; }
    bool hasChildren() const override { return first_child_ != nullptr; }

    Node* appendChild(Node* child) override;
    Node* removeChild(Node* child) override;
    Node* insertBefore(Node* new_child, Node* ref_child) override;

    std::string getTextContent() const override;
    void setTextContent(const std::string& text) override;

    size_t getChildCount() const;
    Node* getChildAt(size_t index) const;

protected:
    Node* first_child_;
    Node* last_child_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_CONTAINER_NODE_H_
