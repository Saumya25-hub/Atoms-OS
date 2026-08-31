/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "document.h"
#include "text.h"
#include "third_party/blink/renderer/core/html/parser/html_parser.h"
#include "third_party/blink/renderer/core/bindings/core/v8/script_controller.h"

namespace blink {

Document::Document()
    : ContainerNode(nullptr, kDocumentNode)
    , title_("ATOMS Page")
    , document_element_(nullptr)
    , head_element_(nullptr)
    , body_element_(nullptr)
    , script_controller_(new ScriptController(this))
{
    document_ = this; // self reference
    document_element_ = createElement("html");
    appendChild(document_element_);

    head_element_ = createElement("head");
    document_element_->appendChild(head_element_);

    body_element_ = createElement("body");
    document_element_->appendChild(body_element_);
}

Document::~Document() {
    delete script_controller_;
}

Element* Document::createElement(const std::string& tag_name) {
    return new Element(this, tag_name);
}

Text* Document::createTextNode(const std::string& text) {
    return new Text(this, text);
}

Element* Document::getElementById(const std::string& id) {
    if (document_element_) {
        return document_element_->getElementById(id);
    }
    return nullptr;
}

Element* Document::querySelector(const std::string& selector) {
    if (document_element_) {
        return document_element_->querySelector(selector);
    }
    return nullptr;
}

void Document::parseHTML(const std::string& html_source) {
    HTMLParser parser(this);
    parser.parseDocument(html_source);
}

} // namespace blink
