/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_STORAGE_DOM_STORAGE_LOCAL_STORAGE_MANAGER_H_
#define THIRD_PARTY_CHROMIUM_STORAGE_DOM_STORAGE_LOCAL_STORAGE_MANAGER_H_

#include "storage_namespace.h"
#include "third_party/chromium_net/base/security_origin.h"

namespace storage {

/*
 * LocalStorageManager manages persistent localStorage across all origins.
 *
 * Pipeline:
 *   localStorage.setItem(key, value)
 *     ↓
 *   StorageNamespace (LOCAL) → StorageArea (origin-isolated)
 *     ↓
 *   AtomsStorageVFSAdapter → ATOMS VFS disk file
 *     ↓
 *   /var/storage/local_<origin_hash>.dat
 *
 * Persistence: Data survives browser/OS restarts via ATOMS VFS.
 * Isolation:   Each origin gets its own StorageArea + VFS file.
 */

class LocalStorageManager {
public:
    LocalStorageManager();
    ~LocalStorageManager();

    StorageArea* GetLocalStorage(const net::SecurityOrigin& origin);
    bool PersistOrigin(const net::SecurityOrigin& origin);
    bool LoadOrigin(const net::SecurityOrigin& origin);
    void PersistAll();
    void Clear();

    size_t origin_count() const { return namespace_.origin_count(); }

private:
    StorageNamespace namespace_;
};

} // namespace storage

#endif // THIRD_PARTY_CHROMIUM_STORAGE_DOM_STORAGE_LOCAL_STORAGE_MANAGER_H_
