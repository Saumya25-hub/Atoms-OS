/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "canonical_cookie.h"
#include "userspace/runtime/c/include/ctype.h"

namespace net {

CanonicalCookie::CanonicalCookie()
    : name_("")
    , value_("")
    , domain_("")
    , path_("/")
    , secure_(false)
    , http_only_(false)
{
}

CanonicalCookie::CanonicalCookie(const std::string& name, const std::string& value,
                                 const std::string& domain, const std::string& path,
                                 bool secure, bool http_only)
    : name_(name)
    , value_(value)
    , domain_(domain)
    , path_(path.empty() ? "/" : path)
    , secure_(secure)
    , http_only_(http_only)
{
}

CanonicalCookie::~CanonicalCookie() {}

static std::string Trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && isspace((unsigned char)str[start])) start++;
    size_t end = str.size();
    while (end > start && isspace((unsigned char)str[end - 1])) end--;
    return str.substr(start, end - start);
}

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

CanonicalCookie CanonicalCookie::Create(const GURL& url, const std::string& cookie_line) {
    std::string name = "";
    std::string value = "";
    std::string domain = url.host();
    std::string path = "/";
    bool secure = false;
    bool http_only = false;

    size_t pos = 0;
    bool first_token = true;

    while (pos < cookie_line.size()) {
        size_t semi = cookie_line.find(';', pos);
        std::string token;
        if (semi != (size_t)-1) {
            token = Trim(cookie_line.substr(pos, semi - pos));
            pos = semi + 1;
        } else {
            token = Trim(cookie_line.substr(pos));
            pos = cookie_line.size();
        }

        if (token.empty()) continue;

        size_t eq = token.find('=');
        if (first_token) {
            first_token = false;
            if (eq != (size_t)-1) {
                name = Trim(token.substr(0, eq));
                value = Trim(token.substr(eq + 1));
            } else {
                name = token;
                value = "";
            }
        } else {
            std::string attr_name;
            std::string attr_val = "";
            if (eq != (size_t)-1) {
                attr_name = Trim(token.substr(0, eq));
                attr_val = Trim(token.substr(eq + 1));
            } else {
                attr_name = token;
            }

            if (EqualsIgnoreCase(attr_name, "Domain")) {
                if (!attr_val.empty()) {
                    if (attr_val[0] == '.') domain = attr_val.substr(1);
                    else domain = attr_val;
                }
            } else if (EqualsIgnoreCase(attr_name, "Path")) {
                if (!attr_val.empty()) path = attr_val;
            } else if (EqualsIgnoreCase(attr_name, "Secure")) {
                secure = true;
            } else if (EqualsIgnoreCase(attr_name, "HttpOnly")) {
                http_only = true;
            }
        }
    }

    return CanonicalCookie(name, value, domain, path, secure, http_only);
}

bool CanonicalCookie::IsMatchForURL(const GURL& url) const {
    if (!url.is_valid()) return false;

    // 1. Secure check
    if (secure_ && !url.is_secure()) return false;

    // 2. Domain check (host == domain or host ends with .domain)
    const std::string& h = url.host();
    if (h != domain_) {
        if (h.size() > domain_.size()) {
            size_t diff = h.size() - domain_.size();
            if (h[diff - 1] == '.' && h.substr(diff) == domain_) {
                // Suffix match
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    // 3. Path check
    const std::string& p = url.path();
    if (path_ != "/") {
        if (p.substr(0, path_.size()) != path_) return false;
    }

    return true;
}

} // namespace net
