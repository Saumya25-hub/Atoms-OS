/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "file_reader.h"

namespace blink {

File::File(const std::string& name, const void* data, size_t size, const std::string& type)
    : Blob(data, size, type),
      name_(name) {
}

File::~File() {
}

FileReader::FileReader()
    : ready_state_(FILE_READER_EMPTY) {
}

FileReader::~FileReader() {
}

void FileReader::readAsText(const Blob& blob) {
    ready_state_ = FILE_READER_LOADING;
    if (blob.data() && blob.size() > 0) {
        result_text_ = std::string(reinterpret_cast<const char*>(blob.data()), blob.size());
    } else {
        result_text_.clear();
    }
    ready_state_ = FILE_READER_DONE;
}

void FileReader::readAsDataURL(const Blob& blob) {
    ready_state_ = FILE_READER_LOADING;
    result_text_ = "data:" + blob.type() + ";base64,";
    ready_state_ = FILE_READER_DONE;
}

void FileReader::abort() {
    ready_state_ = FILE_READER_DONE;
}

} // namespace blink
