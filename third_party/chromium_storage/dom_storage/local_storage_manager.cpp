/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "local_storage_manager.h"
#include "third_party/chromium_storage/adapter/atoms_storage_vfs_adapter.h"

namespace storage {

LocalStorageManager::LocalStorageManager()
    : namespace_(STORAGE_TYPE_LOCAL)
{
}

LocalStorageManager::~LocalStorageManager() {
    PersistAll();
}

StorageArea* LocalStorageManager::GetLocalStorage(const net::SecurityOrigin& origin) {
    StorageArea* area = namespace_.GetStorageArea(origin);

    // If area is empty, attempt to load from VFS
    if (area && area->length() == 0) {
        std::vector<StorageEntry> loaded;
        if (AtomsStorageVFS_Load(origin, &loaded)) {
            area->LoadEntries(loaded);
        }
    }

    return area;
}

bool LocalStorageManager::PersistOrigin(const net::SecurityOrigin& origin) {
    StorageArea* area = namespace_.GetStorageArea(origin);
    if (!area) return false;
    return AtomsStorageVFS_Save(origin, area->GetAllEntries());
}

bool LocalStorageManager::LoadOrigin(const net::SecurityOrigin& origin) {
    StorageArea* area = namespace_.GetStorageArea(origin);
    if (!area) return false;

    std::vector<StorageEntry> loaded;
    if (!AtomsStorageVFS_Load(origin, &loaded)) return false;
    area->LoadEntries(loaded);
    return true;
}

void LocalStorageManager::PersistAll() {
    // Persist is handled per-origin when called explicitly.
    // Full enumeration would require namespace iteration which is
    // triggered by the browser shutdown path.
}

void LocalStorageManager::Clear() {
    namespace_.DestroyAll();
}

} // namespace storage
