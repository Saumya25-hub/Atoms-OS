/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_PARSER_HTML_PARSER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_PARSER_HTML_PARSER_H_

#include "userspace/runtime/cpp/include/string"

namespace blink {

class Document;
class ContainerNode;
class Element;

class HTMLParser {
public:
    explicit HTMLParser(Document* document);
    ~HTMLParser();

    void parseDocument(const std::string& html);
    void parseFragment(const std::string& html, ContainerNode* parent);

private:
    Document* document_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_PARSER_HTML_PARSER_H_
