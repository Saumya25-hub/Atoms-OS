/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gurl.h"
#include "userspace/runtime/c/include/ctype.h"
#include "userspace/runtime/c/include/string.h"

namespace net {

GURL::GURL()
    : is_valid_(false)
    , spec_("")
    , scheme_("")
    , host_("")
    , port_(0)
    , path_("/")
    , query_("")
{
}

GURL::GURL(const std::string& url_string) {
    Parse(url_string);
}

GURL::GURL(const char* url_string) {
    if (url_string) {
        Parse(std::string(url_string));
    } else {
        is_valid_ = false;
        port_ = 0;
    }
}

GURL::~GURL() {}

void GURL::Parse(const std::string& url_string) {
    spec_ = url_string;
    is_valid_ = false;
    scheme_ = "";
    host_ = "";
    port_ = 0;
    path_ = "/";
    query_ = "";

    if (url_string.empty()) return;

    // 1. Extract Scheme (e.g. http, https, file, about)
    size_t scheme_end = url_string.find("://");
    if (scheme_end == (size_t)-1) {
        // Check about: or data:
        size_t colon = url_string.find(':');
        if (colon != (size_t)-1) {
            scheme_ = url_string.substr(0, colon);
            path_ = url_string.substr(colon + 1);
            is_valid_ = true;
            return;
        }
        return;
    }

    scheme_ = url_string.substr(0, scheme_end);
    for (size_t i = 0; i < scheme_.size(); i++) {
        if (scheme_[i] >= 'A' && scheme_[i] <= 'Z') {
            scheme_[i] = (char)(scheme_[i] + 32); // lowercase
        }
    }

    size_t rest_start = scheme_end + 3;
    size_t path_start = url_string.find('/', rest_start);
    size_t query_start = url_string.find('?', rest_start);

    std::string host_port_part;
    if (path_start != (size_t)-1) {
        host_port_part = url_string.substr(rest_start, path_start - rest_start);
        if (query_start != (size_t)-1 && query_start > path_start) {
            path_ = url_string.substr(path_start, query_start - path_start);
            query_ = url_string.substr(query_start + 1);
        } else {
            path_ = url_string.substr(path_start);
        }
    } else if (query_start != (size_t)-1) {
        host_port_part = url_string.substr(rest_start, query_start - rest_start);
        path_ = "/";
        query_ = url_string.substr(query_start + 1);
    } else {
        host_port_part = url_string.substr(rest_start);
        path_ = "/";
    }

    // 2. Parse Host and Port
    size_t colon = host_port_part.find(':');
    if (colon != (size_t)-1) {
        host_ = host_port_part.substr(0, colon);
        std::string port_str = host_port_part.substr(colon + 1);
        uint32_t p = 0;
        for (size_t i = 0; i < port_str.size(); i++) {
            if (port_str[i] >= '0' && port_str[i] <= '9') {
                p = p * 10 + (port_str[i] - '0');
            }
        }
        port_ = (uint16_t)p;
    } else {
        host_ = host_port_part;
        if (scheme_ == "http") port_ = 80;
        else if (scheme_ == "https") port_ = 443;
        else port_ = 0;
    }

    for (size_t i = 0; i < host_.size(); i++) {
        if (host_[i] >= 'A' && host_[i] <= 'Z') {
            host_[i] = (char)(host_[i] + 32); // lowercase
        }
    }

    if (!scheme_.empty() && !host_.empty()) {
        is_valid_ = true;
    }
}

std::string GURL::GetOriginString() const {
    if (!is_valid_) return "null";
    std::string origin = scheme_ + "://" + host_;
    if ((scheme_ == "http" && port_ != 80 && port_ != 0) ||
        (scheme_ == "https" && port_ != 443 && port_ != 0)) {
        char port_buf[16];
        int p = (int)port_;
        int i = 0;
        char tmp[16];
        if (p == 0) tmp[i++] = '0';
        while (p > 0) {
            tmp[i++] = (char)('0' + (p % 10));
            p /= 10;
        }
        int pos = 0;
        for (int j = i - 1; j >= 0; j--) port_buf[pos++] = tmp[j];
        port_buf[pos] = '\0';
        origin += ":";
        origin += port_buf;
    }
    return origin;
}

} // namespace net
