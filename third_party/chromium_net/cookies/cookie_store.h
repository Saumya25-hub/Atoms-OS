/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_NET_COOKIES_COOKIE_STORE_H_
#define THIRD_PARTY_CHROMIUM_NET_COOKIES_COOKIE_STORE_H_

#include "canonical_cookie.h"
#include "userspace/runtime/cpp/include/vector"

namespace net {

class CookieStore {
public:
    CookieStore();
    ~CookieStore();

    bool SetCookie(const GURL& url, const std::string& cookie_line);
    std::string GetCookieHeaderForURL(const GURL& url) const;
    void DeleteCookie(const GURL& url, const std::string& name);
    void Clear();

    size_t size() const { return cookies_.size(); }
    const std::vector<CanonicalCookie>& GetAllCookies() const { return cookies_; }

private:
    std::vector<CanonicalCookie> cookies_;
};

} // namespace net

#endif // THIRD_PARTY_CHROMIUM_NET_COOKIES_COOKIE_STORE_H_
