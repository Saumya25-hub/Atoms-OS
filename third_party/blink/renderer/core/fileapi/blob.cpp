/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "blob.h"
#include "userspace/runtime/c/include/stdio.h"

static uint32_t g_blob_url_counter = 1;

namespace blink {

Blob::Blob()
    : type_("text/plain") {
}

Blob::Blob(const void* data, size_t size, const std::string& type)
    : type_(type) {
    if (data && size > 0) {
        const uint8_t* ptr = static_cast<const uint8_t*>(data);
        data_.resize(size);
        for (size_t i = 0; i < size; i++) {
            data_[i] = ptr[i];
        }
    }
}

Blob::~Blob() {
}

Blob Blob::slice(size_t start, size_t end, const std::string& content_type) const {
    if (start >= data_.size()) {
        return Blob(nullptr, 0, content_type.empty() ? type_ : content_type);
    }
    if (end > data_.size()) end = data_.size();
    if (end <= start) {
        return Blob(nullptr, 0, content_type.empty() ? type_ : content_type);
    }

    size_t len = end - start;
    return Blob(&data_[start], len, content_type.empty() ? type_ : content_type);
}

std::string URL::createObjectURL(const Blob& blob) {
    (void)blob;
    char buf[64];
    snprintf(buf, sizeof(buf), "blob:https://atoms.local/%u", g_blob_url_counter++);
    return std::string(buf);
}

void URL::revokeObjectURL(const std::string& url) {
    (void)url;
}

} // namespace blink
