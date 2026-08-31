/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_TEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_TEXT_H_

#include "node.h"

namespace blink {

class Text : public Node {
public:
    Text(Document* document, const std::string& data);
    ~Text() override = default;

    std::string getNodeName() const override { return "#text"; }
    std::string getNodeValue() const override { return data_; }
    void setNodeValue(const std::string& val) override { data_ = val; }

    const std::string& getData() const { return data_; }
    void setData(const std::string& data) { data_ = data; }

    std::string getTextContent() const override { return data_; }
    void setTextContent(const std::string& text) override { data_ = text; }

    size_t length() const { return data_.size(); }

private:
    std::string data_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_TEXT_H_
