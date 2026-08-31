/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_H_

#include "element.h"

namespace blink {

class Text;
class HTMLBodyElement;
class ScriptController;


class Document : public ContainerNode {
public:
    Document();
    ~Document() override;

    std::string getNodeName() const override { return "#document"; }

    Element* createElement(const std::string& tag_name);
    Text* createTextNode(const std::string& text);

    const std::string& getTitle() const { return title_; }
    void setTitle(const std::string& title) { title_ = title; }

    const std::string& getURL() const { return url_; }
    void setURL(const std::string& url) { url_ = url; }

    Element* getDocumentElement() const { return document_element_; }
    void setDocumentElement(Element* el) { document_element_ = el; }

    Element* getBody() const { return body_element_; }
    void setBody(Element* body) { body_element_ = body; }

    Element* getHead() const { return head_element_; }
    void setHead(Element* head) { head_element_ = head; }

    Element* getElementById(const std::string& id);
    Element* querySelector(const std::string& selector);

    ScriptController* getScriptController() { return script_controller_; }

    void parseHTML(const std::string& html_source);

private:
    std::string title_;
    std::string url_;
    Element* document_element_;
    Element* head_element_;
    Element* body_element_;
    ScriptController* script_controller_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_H_
