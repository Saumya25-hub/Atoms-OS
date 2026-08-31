/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_STORAGE_DOM_STORAGE_STORAGE_NAMESPACE_H_
#define THIRD_PARTY_CHROMIUM_STORAGE_DOM_STORAGE_STORAGE_NAMESPACE_H_

#include "storage_area.h"
#include "third_party/chromium_net/base/security_origin.h"
#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"

namespace storage {

/*
 * StorageNamespace isolates StorageAreas by SecurityOrigin.
 *
 * Two namespace types:
 *   - LOCAL:   Persistent across sessions (persisted to VFS)
 *   - SESSION: Volatile, destroyed when tab/browsing context closes
 *
 * Origin isolation guarantee:
 *   https://example.com       → its own StorageArea
 *   http://example.com        → DIFFERENT StorageArea
 *   https://example.com:8443  → DIFFERENT StorageArea
 *   https://sub.example.com   → DIFFERENT StorageArea
 */

enum StorageType {
    STORAGE_TYPE_LOCAL   = 0,
    STORAGE_TYPE_SESSION = 1,
};

class StorageNamespace {
public:
    explicit StorageNamespace(StorageType type);
    ~StorageNamespace();

    StorageType type() const { return type_; }

    StorageArea* GetStorageArea(const net::SecurityOrigin& origin);
    bool HasStorageArea(const net::SecurityOrigin& origin) const;
    void DestroyStorageArea(const net::SecurityOrigin& origin);
    void DestroyAll();

    size_t origin_count() const { return areas_.size(); }

private:
    struct OriginAreaPair {
        net::SecurityOrigin origin;
        StorageArea area;
    };

    StorageType type_;
    std::vector<OriginAreaPair> areas_;
};

} // namespace storage

#endif // THIRD_PARTY_CHROMIUM_STORAGE_DOM_STORAGE_STORAGE_NAMESPACE_H_
