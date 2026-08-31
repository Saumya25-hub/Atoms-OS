/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_ELEMENT_H_

#include "container_node.h"

namespace blink {

class CSSStyleDeclaration;

struct Attribute {
    std::string name;
    std::string value;
};

class Element : public ContainerNode {
public:
    Element(Document* document, const std::string& tag_name);
    ~Element() override;

    std::string getNodeName() const override { return tag_name_; }
    const std::string& getTagName() const { return tag_name_; }

    const std::string& getAttribute(const std::string& name) const;
    void setAttribute(const std::string& name, const std::string& value);
    bool hasAttribute(const std::string& name) const;
    void removeAttribute(const std::string& name);

    std::string getId() const { return getAttribute("id"); }
    void setId(const std::string& id) { setAttribute("id", id); }

    std::string getClassName() const { return getAttribute("class"); }
    void setClassName(const std::string& cls) { setAttribute("class", cls); }

    std::string getInnerHTML() const;
    void setInnerHTML(const std::string& html);

    Element* querySelector(const std::string& selector);
    Element* getElementById(const std::string& id);

    CSSStyleDeclaration* style();

protected:
    std::string tag_name_;
    std::vector<Attribute> attributes_;
    CSSStyleDeclaration* inline_style_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_ELEMENT_H_
