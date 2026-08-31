/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_NET_BASE_GURL_H_
#define THIRD_PARTY_CHROMIUM_NET_BASE_GURL_H_

#include "userspace/runtime/cpp/include/string"
#include <stdint.h>

namespace net {

class GURL {
public:
    GURL();
    explicit GURL(const std::string& url_string);
    GURL(const char* url_string);
    ~GURL();

    bool is_valid() const { return is_valid_; }
    bool is_empty() const { return spec_.empty(); }

    const std::string& spec() const { return spec_; }
    const std::string& scheme() const { return scheme_; }
    const std::string& host() const { return host_; }
    uint16_t port() const { return port_; }
    const std::string& path() const { return path_; }
    const std::string& query() const { return query_; }

    bool is_secure() const { return scheme_ == "https"; }
    bool is_http_or_https() const { return scheme_ == "http" || scheme_ == "https"; }

    std::string GetOriginString() const;

private:
    void Parse(const std::string& url_string);

    bool is_valid_;
    std::string spec_;
    std::string scheme_;
    std::string host_;
    uint16_t port_;
    std::string path_;
    std::string query_;
};

} // namespace net

#endif // THIRD_PARTY_CHROMIUM_NET_BASE_GURL_H_
