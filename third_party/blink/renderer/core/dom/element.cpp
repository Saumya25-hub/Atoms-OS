/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "element.h"
#include "document.h"
#include "third_party/blink/renderer/core/css/css_style_declaration.h"
#include "third_party/blink/renderer/core/html/parser/html_parser.h"

namespace blink {

Element::Element(Document* document, const std::string& tag_name)
    : ContainerNode(document, kElementNode)
    , tag_name_(tag_name)
    , inline_style_(nullptr)
{
}

Element::~Element() {
    if (inline_style_) delete inline_style_;
}

const std::string& Element::getAttribute(const std::string& name) const {
    static const std::string empty = "";
    for (const auto& attr : attributes_) {
        if (attr.name == name) return attr.value;
    }
    return empty;
}

void Element::setAttribute(const std::string& name, const std::string& value) {
    for (auto& attr : attributes_) {
        if (attr.name == name) {
            attr.value = value;
            if (name == "style" && inline_style_) {
                inline_style_->parseDeclaration(value);
            }
            return;
        }
    }
    attributes_.push_back({name, value});
    if (name == "style") {
        style()->parseDeclaration(value);
    }
}

bool Element::hasAttribute(const std::string& name) const {
    for (const auto& attr : attributes_) {
        if (attr.name == name) return true;
    }
    return false;
}

void Element::removeAttribute(const std::string& name) {
    for (size_t i = 0; i < attributes_.size(); i++) {
        if (attributes_[i].name == name) {
            attributes_.erase(attributes_.begin() + i);
            return;
        }
    }
}

CSSStyleDeclaration* Element::style() {
    if (!inline_style_) {
        inline_style_ = new CSSStyleDeclaration();
        const std::string& s = getAttribute("style");
        if (!s.empty()) {
            inline_style_->parseDeclaration(s);
        }
    }
    return inline_style_;
}

std::string Element::getInnerHTML() const {
    return getTextContent();
}

void Element::setInnerHTML(const std::string& html) {
    // Remove all existing children
    while (first_child_) {
        Node* c = removeChild(first_child_);
        delete c;
    }
    // Parse html into children
    HTMLParser parser(getOwnerDocument());
    parser.parseFragment(html, this);
}

Element* Element::querySelector(const std::string& selector) {
    if (selector.empty()) return nullptr;

    // ID selector: #id
    if (selector[0] == '#') {
        std::string target_id = selector.substr(1);
        if (getId() == target_id) return this;
        for (Node* cur = getFirstChild(); cur; cur = cur->getNextSibling()) {
            if (cur->isElementNode()) {
                Element* found = static_cast<Element*>(cur)->querySelector(selector);
                if (found) return found;
            }
        }
        return nullptr;
    }

    // Class selector: .class
    if (selector[0] == '.') {
        std::string target_cls = selector.substr(1);
        if (getClassName() == target_cls) return this;
        for (Node* cur = getFirstChild(); cur; cur = cur->getNextSibling()) {
            if (cur->isElementNode()) {
                Element* found = static_cast<Element*>(cur)->querySelector(selector);
                if (found) return found;
            }
        }
        return nullptr;
    }

    // Tag name selector
    if (getTagName() == selector) return this;
    for (Node* cur = getFirstChild(); cur; cur = cur->getNextSibling()) {
        if (cur->isElementNode()) {
            Element* found = static_cast<Element*>(cur)->querySelector(selector);
            if (found) return found;
        }
    }
    return nullptr;
}

Element* Element::getElementById(const std::string& id) {
    return querySelector("#" + id);
}

} // namespace blink
