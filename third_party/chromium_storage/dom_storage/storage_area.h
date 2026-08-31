/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_STORAGE_DOM_STORAGE_STORAGE_AREA_H_
#define THIRD_PARTY_CHROMIUM_STORAGE_DOM_STORAGE_STORAGE_AREA_H_

#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"
#include "third_party/chromium_net/base/security_origin.h"

namespace storage {

/*
 * StorageArea implements the Web Storage API (localStorage / sessionStorage)
 * for a single origin. Each StorageArea is strictly isolated by SecurityOrigin.
 *
 * Key-value store with:
 *   - Per-origin quota enforcement (default: 5 MB per spec)
 *   - key(), getItem(), setItem(), removeItem(), clear(), length()
 */

struct StorageEntry {
    std::string key;
    std::string value;
};

class StorageArea {
public:
    StorageArea();
    explicit StorageArea(const net::SecurityOrigin& origin);
    ~StorageArea();

    const net::SecurityOrigin& origin() const { return origin_; }

    size_t length() const { return entries_.size(); }
    std::string key(size_t index) const;
    std::string getItem(const std::string& key) const;
    bool setItem(const std::string& key, const std::string& value);
    void removeItem(const std::string& key);
    void clear();

    size_t currentSizeBytes() const;
    bool isOverQuota() const;

    const std::vector<StorageEntry>& GetAllEntries() const { return entries_; }
    void LoadEntries(const std::vector<StorageEntry>& entries);

    static constexpr size_t kDefaultQuotaBytes = 5 * 1024 * 1024; // 5 MB per spec

private:
    net::SecurityOrigin origin_;
    std::vector<StorageEntry> entries_;
    size_t quota_bytes_;
};

} // namespace storage

#endif // THIRD_PARTY_CHROMIUM_STORAGE_DOM_STORAGE_STORAGE_AREA_H_
