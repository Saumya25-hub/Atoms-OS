/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_NET_BASE_SECURITY_ORIGIN_H_
#define THIRD_PARTY_CHROMIUM_NET_BASE_SECURITY_ORIGIN_H_

#include "gurl.h"

namespace net {

class SecurityOrigin {
public:
    SecurityOrigin();
    SecurityOrigin(const std::string& scheme, const std::string& host, uint16_t port);
    ~SecurityOrigin();

    static SecurityOrigin Create(const GURL& url);
    static SecurityOrigin CreateFromString(const std::string& origin_str);

    const std::string& scheme() const { return scheme_; }
    const std::string& host() const { return host_; }
    uint16_t port() const { return port_; }
    bool is_opaque() const { return is_opaque_; }

    bool IsSameOriginWith(const SecurityOrigin& other) const;
    std::string ToString() const;

private:
    std::string scheme_;
    std::string host_;
    uint16_t port_;
    bool is_opaque_;
};

} // namespace net

#endif // THIRD_PARTY_CHROMIUM_NET_BASE_SECURITY_ORIGIN_H_
