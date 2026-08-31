/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "http_cache.h"

namespace net {

HttpCache::HttpCache() {}
HttpCache::~HttpCache() {}

bool HttpCache::Lookup(const std::string& url_key, HttpCacheEntry* out_entry) const {
    for (size_t i = 0; i < entries_.size(); i++) {
        if (entries_[i].valid && entries_[i].url_key == url_key) {
            if (out_entry) *out_entry = entries_[i];
            return true;
        }
    }
    return false;
}

void HttpCache::Store(const std::string& url_key, int status_code,
                      const std::string& headers_raw, const std::string& body,
                      const std::string& etag, const std::string& last_modified) {
    // Update existing entry if present
    for (size_t i = 0; i < entries_.size(); i++) {
        if (entries_[i].url_key == url_key) {
            entries_[i].status_code = status_code;
            entries_[i].response_headers_raw = headers_raw;
            entries_[i].response_body = body;
            entries_[i].etag = etag;
            entries_[i].last_modified = last_modified;
            entries_[i].creation_time = 0; // TODO: real clock
            entries_[i].expiry_time = 0;
            entries_[i].valid = true;
            return;
        }
    }

    // Evict oldest if full
    if (entries_.size() >= kMaxCacheEntries) {
        entries_.erase(entries_.begin());
    }

    HttpCacheEntry entry;
    entry.url_key = url_key;
    entry.status_code = status_code;
    entry.response_headers_raw = headers_raw;
    entry.response_body = body;
    entry.etag = etag;
    entry.last_modified = last_modified;
    entry.creation_time = 0;
    entry.expiry_time = 0;
    entry.valid = true;
    entries_.push_back(entry);
}

void HttpCache::Invalidate(const std::string& url_key) {
    for (size_t i = 0; i < entries_.size(); i++) {
        if (entries_[i].url_key == url_key) {
            entries_[i].valid = false;
            return;
        }
    }
}

void HttpCache::Clear() {
    entries_.clear();
}

bool HttpCache::NeedsRevalidation(const HttpCacheEntry& entry) const {
    // If entry has ETag or Last-Modified, it can be revalidated
    return !entry.etag.empty() || !entry.last_modified.empty();
}

} // namespace net
