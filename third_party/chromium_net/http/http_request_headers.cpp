/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "http_request_headers.h"

namespace net {

HttpRequestHeaders::HttpRequestHeaders() {}
HttpRequestHeaders::~HttpRequestHeaders() {}

static bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return false;
    }
    return true;
}

bool HttpRequestHeaders::GetHeader(const std::string& key, std::string* out) const {
    for (const auto& pair : headers_) {
        if (EqualsIgnoreCase(pair.key, key)) {
            if (out) *out = pair.value;
            return true;
        }
    }
    return false;
}

void HttpRequestHeaders::SetHeader(const std::string& key, const std::string& value) {
    for (auto& pair : headers_) {
        if (EqualsIgnoreCase(pair.key, key)) {
            pair.value = value;
            return;
        }
    }
    headers_.push_back({key, value});
}

void HttpRequestHeaders::RemoveHeader(const std::string& key) {
    for (size_t i = 0; i < headers_.size(); i++) {
        if (EqualsIgnoreCase(headers_[i].key, key)) {
            headers_.erase(headers_.begin() + i);
            return;
        }
    }
}

bool HttpRequestHeaders::HasHeader(const std::string& key) const {
    return GetHeader(key, nullptr);
}

void HttpRequestHeaders::Clear() {
    headers_.clear();
}

std::string HttpRequestHeaders::ToString() const {
    std::string result = "";
    for (const auto& pair : headers_) {
        result += pair.key + ": " + pair.value + "\r\n";
    }
    return result;
}

} // namespace net
