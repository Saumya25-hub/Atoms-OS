/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "session_storage_manager.h"

namespace storage {

SessionStorageManager::SessionStorageManager()
    : namespace_(STORAGE_TYPE_SESSION)
{
}

SessionStorageManager::~SessionStorageManager() {
    // Session storage is volatile — destroyed with the manager
    DestroyAll();
}

StorageArea* SessionStorageManager::GetSessionStorage(const net::SecurityOrigin& origin) {
    return namespace_.GetStorageArea(origin);
}

void SessionStorageManager::DestroyOrigin(const net::SecurityOrigin& origin) {
    namespace_.DestroyStorageArea(origin);
}

void SessionStorageManager::DestroyAll() {
    namespace_.DestroyAll();
}

} // namespace storage
