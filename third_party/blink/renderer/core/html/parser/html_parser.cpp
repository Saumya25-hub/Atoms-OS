/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "html_parser.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/text.h"
#include "third_party/blink/renderer/core/bindings/core/v8/script_controller.h"
#include "userspace/runtime/c/include/ctype.h"

namespace blink {

HTMLParser::HTMLParser(Document* document)
    : document_(document)
{
}

HTMLParser::~HTMLParser() {}

static void SkipWhitespace(const std::string& str, size_t& pos) {
    while (pos < str.size() && isspace((unsigned char)str[pos])) pos++;
}

void HTMLParser::parseDocument(const std::string& html) {
    if (!document_) return;
    Element* body = document_->getBody();
    if (!body) body = document_->getDocumentElement();
    parseFragment(html, body);
}

void HTMLParser::parseFragment(const std::string& html, ContainerNode* root_target) {
    if (!root_target || !document_) return;

    std::vector<ContainerNode*> stack;
    stack.push_back(root_target);

    size_t pos = 0;
    while (pos < html.size()) {
        if (html[pos] == '<') {
            // Check comment: <!-- ... -->
            if (html.substr(pos, 4) == "<!--") {
                size_t end_comment = html.find("-->", pos + 4);
                if (end_comment != (size_t)-1) {
                    pos = end_comment + 3;
                } else {
                    pos = html.size();
                }
                continue;
            }

            // Check closing tag: </tag>
            if (pos + 1 < html.size() && html[pos + 1] == '/') {
                pos += 2; // skip '</'
                size_t tag_start = pos;
                while (pos < html.size() && isalnum((unsigned char)html[pos])) pos++;
                std::string tag_name = html.substr(tag_start, pos - tag_start);
                while (pos < html.size() && html[pos] != '>') pos++;
                if (pos < html.size()) pos++; // skip '>'

                // Pop stack until matching tag
                if (stack.size() > 1) {
                    for (int i = (int)stack.size() - 1; i >= 1; i--) {
                        if (stack[i]->getNodeName() == tag_name) {
                            stack.resize(i);
                            break;
                        }
                    }
                }
                continue;
            }

            // Opening tag: <tag ...>
            pos++; // skip '<'
            SkipWhitespace(html, pos);
            size_t tag_start = pos;
            while (pos < html.size() && (isalnum((unsigned char)html[pos]) || html[pos] == '-')) pos++;
            std::string tag_name = html.substr(tag_start, pos - tag_start);
            if (tag_name.empty()) continue;

            Element* el = document_->createElement(tag_name);

            // Parse attributes: name="value"
            while (pos < html.size() && html[pos] != '>' && html[pos] != '/') {
                SkipWhitespace(html, pos);
                if (pos >= html.size() || html[pos] == '>' || html[pos] == '/') break;

                size_t attr_name_start = pos;
                while (pos < html.size() && (isalnum((unsigned char)html[pos]) || html[pos] == '-' || html[pos] == '_')) pos++;
                std::string attr_name = html.substr(attr_name_start, pos - attr_name_start);

                SkipWhitespace(html, pos);
                std::string attr_val = "";
                if (pos < html.size() && html[pos] == '=') {
                    pos++; // skip '='
                    SkipWhitespace(html, pos);
                    if (pos < html.size() && (html[pos] == '"' || html[pos] == '\'')) {
                        char quote = html[pos++];
                        size_t val_start = pos;
                        while (pos < html.size() && html[pos] != quote) pos++;
                        attr_val = html.substr(val_start, pos - val_start);
                        if (pos < html.size()) pos++; // skip quote
                    } else {
                        size_t val_start = pos;
                        while (pos < html.size() && !isspace((unsigned char)html[pos]) && html[pos] != '>') pos++;
                        attr_val = html.substr(val_start, pos - val_start);
                    }
                }
                if (!attr_name.empty()) {
                    el->setAttribute(attr_name, attr_val);
                }
            }

            bool self_closing = false;
            if (pos < html.size() && html[pos] == '/') {
                self_closing = true;
                pos++;
            }
            if (pos < html.size() && html[pos] == '>') pos++;

            // Handle special tag: <script>
            if (tag_name == "script") {
                size_t end_script = html.find("</script>", pos);
                std::string script_code;
                if (end_script != (size_t)-1) {
                    script_code = html.substr(pos, end_script - pos);
                    pos = end_script + 9;
                } else {
                    script_code = html.substr(pos);
                    pos = html.size();
                }
                // Execute script immediately in document context
                if (document_->getScriptController()) {
                    document_->getScriptController()->executeScript(script_code);
                }
                continue;
            }

            // Append element to current top of stack
            ContainerNode* current_parent = stack.back();
            current_parent->appendChild(el);

            // Void tags do not push to stack
            if (!self_closing && tag_name != "br" && tag_name != "img" && tag_name != "meta" && tag_name != "link" && tag_name != "hr") {
                stack.push_back(el);
            }
        } else {
            // Text node
            size_t text_start = pos;
            while (pos < html.size() && html[pos] != '<') pos++;
            std::string text_str = html.substr(text_start, pos - text_start);
            // If text contains non-whitespace, append Text node
            bool has_content = false;
            for (size_t k = 0; k < text_str.size(); k++) {
                char c = text_str[k];
                if (!isspace((unsigned char)c)) { has_content = true; break; }
            }
            if (has_content) {
                stack.back()->appendChild(document_->createTextNode(text_str));
            }

        }
    }
}

} // namespace blink
