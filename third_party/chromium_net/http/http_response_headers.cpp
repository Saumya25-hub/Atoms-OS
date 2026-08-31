/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "http_response_headers.h"
#include "userspace/runtime/c/include/ctype.h"
#include "userspace/runtime/c/include/stdlib.h"

namespace net {

HttpResponseHeaders::HttpResponseHeaders()
    : response_code_(0)
    , status_line_("")
    , raw_headers_("")
{
}

HttpResponseHeaders::HttpResponseHeaders(const std::string& raw_headers) {
    ParseRawHeaders(raw_headers);
}

HttpResponseHeaders::~HttpResponseHeaders() {}

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

static std::string Trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && isspace((unsigned char)str[start])) start++;
    size_t end = str.size();
    while (end > start && isspace((unsigned char)str[end - 1])) end--;
    return str.substr(start, end - start);
}

void HttpResponseHeaders::ParseRawHeaders(const std::string& raw) {
    raw_headers_ = raw;
    response_code_ = 0;
    status_line_ = "";
    headers_.clear();

    if (raw.empty()) return;

    size_t line_start = 0;
    bool is_first_line = true;

    while (line_start < raw.size()) {
        size_t line_end = raw.find("\r\n", line_start);
        std::string line;
        if (line_end != (size_t)-1) {
            line = raw.substr(line_start, line_end - line_start);
            line_start = line_end + 2;
        } else {
            line = raw.substr(line_start);
            line_start = raw.size();
        }

        if (line.empty()) break;

        if (is_first_line) {
            status_line_ = line;
            is_first_line = false;

            // Parse status code: HTTP/1.1 200 OK
            size_t first_space = line.find(' ');
            if (first_space != (size_t)-1) {
                size_t second_space = line.find(' ', first_space + 1);
                std::string code_str;
                if (second_space != (size_t)-1) {
                    code_str = line.substr(first_space + 1, second_space - (first_space + 1));
                } else {
                    code_str = line.substr(first_space + 1);
                }
                int code = 0;
                for (size_t i = 0; i < code_str.size(); i++) {
                    if (code_str[i] >= '0' && code_str[i] <= '9') {
                        code = code * 10 + (code_str[i] - '0');
                    }
                }
                response_code_ = code;
            }
        } else {
            size_t colon = line.find(':');
            if (colon != (size_t)-1) {
                std::string key = Trim(line.substr(0, colon));
                std::string val = Trim(line.substr(colon + 1));
                headers_.push_back({key, val});
            }
        }
    }
}

bool HttpResponseHeaders::GetNormalizedHeader(const std::string& key, std::string* out) const {
    for (const auto& pair : headers_) {
        if (EqualsIgnoreCase(pair.key, key)) {
            if (out) *out = pair.value;
            return true;
        }
    }
    return false;
}

bool HttpResponseHeaders::HasHeader(const std::string& key) const {
    return GetNormalizedHeader(key, nullptr);
}

int64_t HttpResponseHeaders::GetContentLength() const {
    std::string len_str;
    if (GetNormalizedHeader("Content-Length", &len_str)) {
        int64_t val = 0;
        for (size_t i = 0; i < len_str.size(); i++) {
            if (len_str[i] >= '0' && len_str[i] <= '9') {
                val = val * 10 + (len_str[i] - '0');
            }
        }
        return val;
    }
    return -1;
}

bool HttpResponseHeaders::IsRedirect() const {
    return response_code_ == 301 || response_code_ == 302 ||
           response_code_ == 307 || response_code_ == 308 ||
           response_code_ == 303;
}

bool HttpResponseHeaders::GetLocationHeader(std::string* out_location) const {
    return GetNormalizedHeader("Location", out_location);
}

std::vector<std::string> HttpResponseHeaders::GetSetCookieHeaders() const {
    std::vector<std::string> cookies;
    for (const auto& pair : headers_) {
        if (EqualsIgnoreCase(pair.key, "Set-Cookie")) {
            cookies.push_back(pair.value);
        }
    }
    return cookies;
}

} // namespace net
