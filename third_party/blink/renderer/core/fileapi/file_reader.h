/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FILEAPI_FILE_READER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FILEAPI_FILE_READER_H_

#include "blob.h"
#include "userspace/runtime/cpp/include/string"

namespace blink {

class File : public Blob {
public:
    File(const std::string& name, const void* data, size_t size, const std::string& type = "text/plain");
    ~File();

    const std::string& name() const { return name_; }

private:
    std::string name_;
};

enum FileReaderState {
    FILE_READER_EMPTY = 0,
    FILE_READER_LOADING = 1,
    FILE_READER_DONE = 2
};

class FileReader {
public:
    FileReader();
    ~FileReader();

    FileReaderState readyState() const { return ready_state_; }
    const std::string& result() const { return result_text_; }

    void readAsText(const Blob& blob);
    void readAsDataURL(const Blob& blob);
    void abort();

private:
    FileReaderState ready_state_;
    std::string result_text_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_FILEAPI_FILE_READER_H_
