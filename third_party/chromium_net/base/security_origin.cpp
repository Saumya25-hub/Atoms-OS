/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "security_origin.h"

namespace net {

SecurityOrigin::SecurityOrigin()
    : scheme_("")
    , host_("")
    , port_(0)
    , is_opaque_(true)
{
}

SecurityOrigin::SecurityOrigin(const std::string& scheme, const std::string& host, uint16_t port)
    : scheme_(scheme)
    , host_(host)
    , port_(port)
    , is_opaque_(false)
{
}

SecurityOrigin::~SecurityOrigin() {}

SecurityOrigin SecurityOrigin::Create(const GURL& url) {
    if (!url.is_valid() || !url.is_http_or_https()) {
        return SecurityOrigin();
    }
    return SecurityOrigin(url.scheme(), url.host(), url.port());
}

SecurityOrigin SecurityOrigin::CreateFromString(const std::string& origin_str) {
    GURL url(origin_str);
    return Create(url);
}

bool SecurityOrigin::IsSameOriginWith(const SecurityOrigin& other) const {
    if (is_opaque_ || other.is_opaque_) return false;
    return scheme_ == other.scheme_ &&
           host_ == other.host_ &&
           port_ == other.port_;
}

std::string SecurityOrigin::ToString() const {
    if (is_opaque_) return "null";
    std::string s = scheme_ + "://" + host_;
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
        s += ":";
        s += port_buf;
    }
    return s;
}

} // namespace net
