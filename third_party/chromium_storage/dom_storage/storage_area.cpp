/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "storage_area.h"

namespace storage {

StorageArea::StorageArea()
    : origin_()
    , quota_bytes_(kDefaultQuotaBytes)
{
}

StorageArea::StorageArea(const net::SecurityOrigin& origin)
    : origin_(origin)
    , quota_bytes_(kDefaultQuotaBytes)
{
}

StorageArea::~StorageArea() {}

std::string StorageArea::key(size_t index) const {
    if (index >= entries_.size()) return "";
    return entries_[index].key;
}

std::string StorageArea::getItem(const std::string& key) const {
    for (size_t i = 0; i < entries_.size(); i++) {
        if (entries_[i].key == key) return entries_[i].value;
    }
    return "";
}

bool StorageArea::setItem(const std::string& key, const std::string& value) {
    // Check if updating existing key
    for (size_t i = 0; i < entries_.size(); i++) {
        if (entries_[i].key == key) {
            // Calculate new size: subtract old, add new
            size_t old_entry_size = entries_[i].key.size() + entries_[i].value.size();
            size_t new_entry_size = key.size() + value.size();
            size_t current = currentSizeBytes();
            if (current - old_entry_size + new_entry_size > quota_bytes_) return false;
            entries_[i].value = value;
            return true;
        }
    }

    // New entry — check quota
    size_t new_entry_size = key.size() + value.size();
    if (currentSizeBytes() + new_entry_size > quota_bytes_) return false;

    StorageEntry entry;
    entry.key = key;
    entry.value = value;
    entries_.push_back(entry);
    return true;
}

void StorageArea::removeItem(const std::string& key) {
    for (size_t i = 0; i < entries_.size(); i++) {
        if (entries_[i].key == key) {
            entries_.erase(entries_.begin() + i);
            return;
        }
    }
}

void StorageArea::clear() {
    entries_.clear();
}

size_t StorageArea::currentSizeBytes() const {
    size_t total = 0;
    for (size_t i = 0; i < entries_.size(); i++) {
        total += entries_[i].key.size() + entries_[i].value.size();
    }
    return total;
}

bool StorageArea::isOverQuota() const {
    return currentSizeBytes() > quota_bytes_;
}

void StorageArea::LoadEntries(const std::vector<StorageEntry>& entries) {
    entries_ = entries;
}

} // namespace storage
