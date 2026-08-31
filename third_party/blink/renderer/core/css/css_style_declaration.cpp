/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "css_style_declaration.h"
#include "userspace/runtime/c/include/ctype.h"
#include "userspace/runtime/c/include/stdlib.h"
#include "userspace/runtime/c/include/string.h"

namespace blink {

CSSStyleDeclaration::CSSStyleDeclaration() {}
CSSStyleDeclaration::~CSSStyleDeclaration() {}

std::string CSSStyleDeclaration::getPropertyValue(const std::string& property_name) const {
    for (const auto& prop : properties_) {
        if (prop.name == property_name) return prop.value;
    }
    return "";
}

void CSSStyleDeclaration::setProperty(const std::string& property_name, const std::string& value) {
    for (auto& prop : properties_) {
        if (prop.name == property_name) {
            prop.value = value;
            return;
        }
    }
    properties_.push_back({property_name, value});
}

void CSSStyleDeclaration::removeProperty(const std::string& property_name) {
    for (size_t i = 0; i < properties_.size(); i++) {
        if (properties_[i].name == property_name) {
            properties_.erase(properties_.begin() + i);
            return;
        }
    }
}

static std::string Trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && isspace((unsigned char)str[start])) start++;
    size_t end = str.size();
    while (end > start && isspace((unsigned char)str[end - 1])) end--;
    return str.substr(start, end - start);
}

void CSSStyleDeclaration::parseDeclaration(const std::string& css_text) {
    size_t pos = 0;
    while (pos < css_text.size()) {
        size_t colon = css_text.find(':', pos);
        if (colon == (size_t)-1) break;

        std::string name = Trim(css_text.substr(pos, colon - pos));
        size_t semi = css_text.find(';', colon + 1);
        std::string value;
        if (semi != (size_t)-1) {
            value = Trim(css_text.substr(colon + 1, semi - (colon + 1)));
            pos = semi + 1;
        } else {
            value = Trim(css_text.substr(colon + 1));
            pos = css_text.size();
        }

        if (!name.empty() && !value.empty()) {
            setProperty(name, value);
        }
    }
}

std::string CSSStyleDeclaration::getCssText() const {
    std::string res = "";
    for (const auto& prop : properties_) {
        res += prop.name + ": " + prop.value + "; ";
    }
    return res;
}

static uint32_t ParseHex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return 0;
}

static uint32_t ParseColor(const std::string& val, uint32_t default_color) {
    if (val.empty()) return default_color;
    if (val == "black") return 0xFF000000;
    if (val == "white") return 0xFFFFFFFF;
    if (val == "red") return 0xFFFF0000;
    if (val == "green") return 0xFF00FF00;
    if (val == "blue") return 0xFF0000FF;
    if (val == "yellow") return 0xFFFFFF00;
    if (val == "transparent") return 0x00000000;

    if (val[0] == '#') {
        if (val.size() == 7) { // #RRGGBB
            uint32_t r = (ParseHex(val[1]) << 4) | ParseHex(val[2]);
            uint32_t g = (ParseHex(val[3]) << 4) | ParseHex(val[4]);
            uint32_t b = (ParseHex(val[5]) << 4) | ParseHex(val[6]);
            return 0xFF000000 | (r << 16) | (g << 8) | b;
        } else if (val.size() == 4) { // #RGB
            uint32_t r = ParseHex(val[1]) * 17;
            uint32_t g = ParseHex(val[2]) * 17;
            uint32_t b = ParseHex(val[3]) * 17;
            return 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
    return default_color;
}

uint32_t CSSStyleDeclaration::getColor(uint32_t default_color) const {
    std::string val = getPropertyValue("color");
    return ParseColor(val, default_color);
}

uint32_t CSSStyleDeclaration::getBackgroundColor(uint32_t default_bg) const {
    std::string val = getPropertyValue("background-color");
    if (val.empty()) val = getPropertyValue("background");
    return ParseColor(val, default_bg);
}

static int ParsePx(const std::string& val, int default_val) {
    if (val.empty()) return default_val;
    const char* s = val.c_str();
    while (*s == ' ' || *s == '\t') s++;
    int sign = 1;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') { s++; }
    if (!(*s >= '0' && *s <= '9')) return default_val;
    int num = 0;
    while (*s >= '0' && *s <= '9') {
        num = num * 10 + (*s - '0');
        s++;
    }
    return num * sign;
}


int CSSStyleDeclaration::getFontSize(int default_size) const {
    return ParsePx(getPropertyValue("font-size"), default_size);
}

int CSSStyleDeclaration::getMargin(int default_margin) const {
    return ParsePx(getPropertyValue("margin"), default_margin);
}

int CSSStyleDeclaration::getPadding(int default_padding) const {
    return ParsePx(getPropertyValue("padding"), default_padding);
}

int CSSStyleDeclaration::getWidth(int default_width) const {
    return ParsePx(getPropertyValue("width"), default_width);
}

int CSSStyleDeclaration::getHeight(int default_height) const {
    return ParsePx(getPropertyValue("height"), default_height);
}

bool CSSStyleDeclaration::isBlock() const {
    std::string d = getPropertyValue("display");
    return d == "block" || d.empty();
}

bool CSSStyleDeclaration::isInline() const {
    return getPropertyValue("display") == "inline";
}

bool CSSStyleDeclaration::isHidden() const {
    return getPropertyValue("display") == "none";
}

} // namespace blink
