/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_STYLE_DECLARATION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_STYLE_DECLARATION_H_

#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"
#include <stdint.h>

namespace blink {

struct CSSProperty {
    std::string name;
    std::string value;
};

class CSSStyleDeclaration {
public:
    CSSStyleDeclaration();
    ~CSSStyleDeclaration();

    std::string getPropertyValue(const std::string& property_name) const;
    void setProperty(const std::string& property_name, const std::string& value);
    void removeProperty(const std::string& property_name);

    void parseDeclaration(const std::string& css_text);
    std::string getCssText() const;

    // Parsed style accessors
    uint32_t getColor(uint32_t default_color = 0xFFFFFFFF) const;
    uint32_t getBackgroundColor(uint32_t default_bg = 0x00000000) const;
    int getFontSize(int default_size = 14) const;
    int getMargin(int default_margin = 0) const;
    int getPadding(int default_padding = 0) const;
    int getWidth(int default_width = -1) const;
    int getHeight(int default_height = -1) const;
    bool isBlock() const;
    bool isInline() const;
    bool isHidden() const;

private:
    std::vector<CSSProperty> properties_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_STYLE_DECLARATION_H_
