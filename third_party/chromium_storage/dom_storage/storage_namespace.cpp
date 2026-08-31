/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "storage_namespace.h"

namespace storage {

StorageNamespace::StorageNamespace(StorageType type)
    : type_(type)
{
}

StorageNamespace::~StorageNamespace() {}

StorageArea* StorageNamespace::GetStorageArea(const net::SecurityOrigin& origin) {
    // Search for existing area with matching origin
    for (size_t i = 0; i < areas_.size(); i++) {
        if (areas_[i].origin.IsSameOriginWith(origin)) {
            return &areas_[i].area;
        }
    }

    // Create new StorageArea for this origin
    OriginAreaPair pair;
    pair.origin = origin;
    pair.area = StorageArea(origin);
    areas_.push_back(pair);
    return &areas_[areas_.size() - 1].area;
}

bool StorageNamespace::HasStorageArea(const net::SecurityOrigin& origin) const {
    for (size_t i = 0; i < areas_.size(); i++) {
        if (areas_[i].origin.IsSameOriginWith(origin)) {
            return true;
        }
    }
    return false;
}

void StorageNamespace::DestroyStorageArea(const net::SecurityOrigin& origin) {
    for (size_t i = 0; i < areas_.size(); i++) {
        if (areas_[i].origin.IsSameOriginWith(origin)) {
            areas_.erase(areas_.begin() + i);
            return;
        }
    }
}

void StorageNamespace::DestroyAll() {
    areas_.clear();
}

} // namespace storage
