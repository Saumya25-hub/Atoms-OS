/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_NET_COOKIES_CANONICAL_COOKIE_H_
#define THIRD_PARTY_CHROMIUM_NET_COOKIES_CANONICAL_COOKIE_H_

#include "userspace/runtime/cpp/include/string"
#include "third_party/chromium_net/base/gurl.h"

namespace net {

class CanonicalCookie {
public:
    CanonicalCookie();
    CanonicalCookie(const std::string& name, const std::string& value,
                    const std::string& domain, const std::string& path,
                    bool secure, bool http_only);
    ~CanonicalCookie();

    static CanonicalCookie Create(const GURL& url, const std::string& cookie_line);

    const std::string& Name() const { return name_; }
    const std::string& Value() const { return value_; }
    const std::string& Domain() const { return domain_; }
    const std::string& Path() const { return path_; }
    bool IsSecure() const { return secure_; }
    bool IsHttpOnly() const { return http_only_; }

    bool IsMatchForURL(const GURL& url) const;

private:
    std::string name_;
    std::string value_;
    std::string domain_;
    std::string path_;
    bool secure_;
    bool http_only_;
};

} // namespace net

#endif // THIRD_PARTY_CHROMIUM_NET_COOKIES_CANONICAL_COOKIE_H_
