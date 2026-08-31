/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "shared_buffer.h"
#include "userspace/runtime/c/include/stdlib.h"
#include "userspace/runtime/c/include/string.h"

namespace mojo {
namespace core {

SharedBufferDispatcher::SharedBufferDispatcher(uint64_t num_bytes, uint32_t owner_pid)
    : size_(num_bytes),
      owner_pid_(owner_pid),
      buffer_memory_(nullptr),
      is_closed_(false),
      map_count_(0) {
    if (num_bytes > 0) {
        buffer_memory_ = malloc((size_t)num_bytes);
        if (buffer_memory_) {
            memset(buffer_memory_, 0, (size_t)num_bytes);
        }
    }
}

SharedBufferDispatcher::~SharedBufferDispatcher() {
    Close();
}

SharedBufferDispatcher* SharedBufferDispatcher::Create(uint64_t num_bytes, uint32_t owner_pid) {
    if (num_bytes == 0 || num_bytes > (64 * 1024 * 1024)) { // 64MB Cap
        return nullptr;
    }
    return new SharedBufferDispatcher(num_bytes, owner_pid);
}

void SharedBufferDispatcher::Close() {
    if (is_closed_) return;
    is_closed_ = true;

    if (buffer_memory_ && map_count_ == 0) {
        free(buffer_memory_);
        buffer_memory_ = nullptr;
    }
}

MojoResult SharedBufferDispatcher::Map(uint64_t offset, uint64_t num_bytes, void** out_ptr) {
    if (is_closed_ || !buffer_memory_ || !out_ptr) return MOJO_RESULT_INVALID_ARGUMENT;
    if (offset + num_bytes > size_) return MOJO_RESULT_OUT_OF_RANGE;

    uint8_t* base = static_cast<uint8_t*>(buffer_memory_);
    *out_ptr = static_cast<void*>(base + offset);
    map_count_++;
    return MOJO_RESULT_OK;
}

MojoResult SharedBufferDispatcher::Unmap(void* ptr) {
    if (!ptr) return MOJO_RESULT_INVALID_ARGUMENT;
    if (map_count_ > 0) map_count_--;

    if (is_closed_ && map_count_ == 0 && buffer_memory_) {
        free(buffer_memory_);
        buffer_memory_ = nullptr;
    }
    return MOJO_RESULT_OK;
}

SharedBufferDispatcher* SharedBufferDispatcher::Duplicate() {
    if (is_closed_ || !buffer_memory_) return nullptr;

    SharedBufferDispatcher* dup = new SharedBufferDispatcher(size_, owner_pid_);
    if (dup && dup->buffer_memory_ && buffer_memory_) {
        memcpy(dup->buffer_memory_, buffer_memory_, (size_t)size_);
    }
    return dup;
}

} // namespace core
} // namespace mojo
