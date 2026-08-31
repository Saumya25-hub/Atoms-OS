/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cookie_store.h"

namespace net {

CookieStore::CookieStore() {}
CookieStore::~CookieStore() {}

bool CookieStore::SetCookie(const GURL& url, const std::string& cookie_line) {
    if (!url.is_valid() || cookie_line.empty()) return false;

    CanonicalCookie cookie = CanonicalCookie::Create(url, cookie_line);
    if (cookie.Name().empty()) return false;

    // Replace existing cookie with same name+domain+path
    for (size_t i = 0; i < cookies_.size(); i++) {
        if (cookies_[i].Name() == cookie.Name() &&
            cookies_[i].Domain() == cookie.Domain() &&
            cookies_[i].Path() == cookie.Path()) {
            cookies_[i] = cookie;
            return true;
        }
    }
    cookies_.push_back(cookie);
    return true;
}

std::string CookieStore::GetCookieHeaderForURL(const GURL& url) const {
    std::string header = "";
    bool first = true;
    for (size_t i = 0; i < cookies_.size(); i++) {
        if (cookies_[i].IsMatchForURL(url)) {
            if (!first) header += "; ";
            header += cookies_[i].Name() + "=" + cookies_[i].Value();
            first = false;
        }
    }
    return header;
}

void CookieStore::DeleteCookie(const GURL& url, const std::string& name) {
    for (size_t i = 0; i < cookies_.size(); i++) {
        if (cookies_[i].Name() == name && cookies_[i].IsMatchForURL(url)) {
            cookies_.erase(cookies_.begin() + i);
            return;
        }
    }
}

void CookieStore::Clear() {
    cookies_.clear();
}

} // namespace net
