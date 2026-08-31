/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FILEAPI_BLOB_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FILEAPI_BLOB_H_

#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"
#include <stdint.h>
#include <stddef.h>

namespace blink {

class Blob {
public:
    Blob();
    Blob(const void* data, size_t size, const std::string& type = "text/plain");
    ~Blob();

    size_t size() const { return data_.size(); }
    const std::string& type() const { return type_; }
    const uint8_t* data() const { return data_.empty() ? nullptr : &data_[0]; }

    Blob slice(size_t start, size_t end, const std::string& content_type = "") const;

private:
    std::vector<uint8_t> data_;
    std::string type_;
};

class URL {
public:
    static std::string createObjectURL(const Blob& blob);
    static void revokeObjectURL(const std::string& url);
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_FILEAPI_BLOB_H_
