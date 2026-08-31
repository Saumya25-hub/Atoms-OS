/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_H_

#include <stddef.h>
#include <stdint.h>
#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"

namespace blink {

class Document;
class Element;
class ContainerNode;
class LayoutObject;

enum NodeType {
    kElementNode = 1,
    kAttributeNode = 2,
    kTextNode = 3,
    kCDataSectionNode = 4,
    kCommentNode = 8,
    kDocumentNode = 9,
    kDocumentTypeNode = 10,
    kDocumentFragmentNode = 11
};

class Node {
public:
    Node(Document* document, NodeType type);
    virtual ~Node();

    NodeType getNodeType() const { return type_; }
    virtual std::string getNodeName() const = 0;
    virtual std::string getNodeValue() const { return ""; }
    virtual void setNodeValue(const std::string& val) { (void)val; }

    Document* getOwnerDocument() const { return document_; }
    ContainerNode* getParentNode() const { return parent_node_; }
    void setParentNode(ContainerNode* parent) { parent_node_ = parent; }

    Node* getPreviousSibling() const { return previous_sibling_; }
    Node* getNextSibling() const { return next_sibling_; }
    void setPreviousSibling(Node* prev) { previous_sibling_ = prev; }
    void setNextSibling(Node* next) { next_sibling_ = next; }

    virtual Node* getFirstChild() const { return nullptr; }
    virtual Node* getLastChild() const { return nullptr; }
    virtual bool hasChildren() const { return false; }

    virtual Node* appendChild(Node* child);
    virtual Node* removeChild(Node* child);
    virtual Node* insertBefore(Node* new_child, Node* ref_child);

    virtual std::string getTextContent() const;
    virtual void setTextContent(const std::string& text);

    LayoutObject* getLayoutObject() const { return layout_object_; }
    void setLayoutObject(LayoutObject* layout) { layout_object_ = layout; }

    bool isElementNode() const { return type_ == kElementNode; }
    bool isTextNode() const { return type_ == kTextNode; }
    bool isDocumentNode() const { return type_ == kDocumentNode; }

protected:
    Document* document_;
    NodeType type_;
    ContainerNode* parent_node_;
    Node* previous_sibling_;
    Node* next_sibling_;
    LayoutObject* layout_object_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_H_
