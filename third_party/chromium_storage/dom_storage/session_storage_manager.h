/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_STORAGE_DOM_STORAGE_SESSION_STORAGE_MANAGER_H_
#define THIRD_PARTY_CHROMIUM_STORAGE_DOM_STORAGE_SESSION_STORAGE_MANAGER_H_

#include "storage_namespace.h"
#include "third_party/chromium_net/base/security_origin.h"

namespace storage {

/*
 * SessionStorageManager manages volatile sessionStorage.
 *
 * Unlike localStorage, sessionStorage is NOT persisted to VFS.
 * It is destroyed when the browsing context (tab) is closed.
 *
 * Origin isolation is enforced identically to localStorage.
 */

class SessionStorageManager {
public:
    SessionStorageManager();
    ~SessionStorageManager();

    StorageArea* GetSessionStorage(const net::SecurityOrigin& origin);
    void DestroyOrigin(const net::SecurityOrigin& origin);
    void DestroyAll();

    size_t origin_count() const { return namespace_.origin_count(); }

private:
    StorageNamespace namespace_;
};

} // namespace storage

#endif // THIRD_PARTY_CHROMIUM_STORAGE_DOM_STORAGE_SESSION_STORAGE_MANAGER_H_
