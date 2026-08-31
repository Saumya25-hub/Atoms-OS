/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_CPP_SYSTEM_BUFFER_H_
#define MOJO_PUBLIC_CPP_SYSTEM_BUFFER_H_

#include "handle.h"
#include "mojo/public/c/system/buffer.h"

namespace mojo {

class SharedBufferHandle : public Handle {
public:
    SharedBufferHandle() : Handle() {}
    explicit SharedBufferHandle(MojoHandle value) : Handle(value) {}

    MojoResult Map(uint64_t offset, uint64_t num_bytes, void** out_ptr) const {
        return MojoMapBuffer(value_, offset, num_bytes, nullptr, out_ptr);
    }

    MojoResult Duplicate(SharedBufferHandle* out_new_handle) const {
        MojoHandle nh = MOJO_HANDLE_INVALID;
        MojoResult res = MojoDuplicateBufferHandle(value_, nullptr, &nh);
        if (res == MOJO_RESULT_OK && out_new_handle) {
            *out_new_handle = SharedBufferHandle(nh);
        }
        return res;
    }
};

using ScopedSharedBufferHandle = ScopedHandleBase<SharedBufferHandle>;

class ScopedSharedBufferMapping {
public:
    ScopedSharedBufferMapping() : buffer_(nullptr) {}
    explicit ScopedSharedBufferMapping(void* buffer) : buffer_(buffer) {}
    ~ScopedSharedBufferMapping() { reset(); }

    ScopedSharedBufferMapping(ScopedSharedBufferMapping&& other) : buffer_(other.release()) {}
    ScopedSharedBufferMapping& operator=(ScopedSharedBufferMapping&& other) {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    ScopedSharedBufferMapping(const ScopedSharedBufferMapping&) = delete;
    ScopedSharedBufferMapping& operator=(const ScopedSharedBufferMapping&) = delete;

    void* get() const { return buffer_; }
    bool is_valid() const { return buffer_ != nullptr; }

    void reset(void* buffer = nullptr) {
        if (buffer_) {
            MojoUnmapBuffer(buffer_);
        }
        buffer_ = buffer;
    }

    void* release() {
        void* unused = buffer_;
        buffer_ = nullptr;
        return unused;
    }

private:
    void* buffer_;
};

inline ScopedSharedBufferHandle SharedBufferCreate(uint64_t num_bytes) {
    MojoHandle h = MOJO_HANDLE_INVALID;
    MojoResult res = MojoCreateSharedBuffer(num_bytes, nullptr, &h);
    if (res == MOJO_RESULT_OK) {
        return ScopedSharedBufferHandle(SharedBufferHandle(h));
    }
    return ScopedSharedBufferHandle();
}

} // namespace mojo

#endif // MOJO_PUBLIC_CPP_SYSTEM_BUFFER_H_
