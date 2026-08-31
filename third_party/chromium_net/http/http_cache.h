/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_NET_HTTP_HTTP_CACHE_H_
#define THIRD_PARTY_CHROMIUM_NET_HTTP_HTTP_CACHE_H_

#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"
#include <stdint.h>

namespace net {

struct HttpCacheEntry {
    std::string url_key;
    int status_code;
    std::string response_headers_raw;
    std::string response_body;
    std::string etag;
    std::string last_modified;
    uint64_t creation_time;
    uint64_t expiry_time;
    bool valid;
};

class HttpCache {
public:
    HttpCache();
    ~HttpCache();

    bool Lookup(const std::string& url_key, HttpCacheEntry* out_entry) const;
    void Store(const std::string& url_key, int status_code,
               const std::string& headers_raw, const std::string& body,
               const std::string& etag, const std::string& last_modified);
    void Invalidate(const std::string& url_key);
    void Clear();

    size_t size() const { return entries_.size(); }

    bool NeedsRevalidation(const HttpCacheEntry& entry) const;

private:
    std::vector<HttpCacheEntry> entries_;
    static constexpr size_t kMaxCacheEntries = 128;
};

} // namespace net

#endif // THIRD_PARTY_CHROMIUM_NET_HTTP_HTTP_CACHE_H_
