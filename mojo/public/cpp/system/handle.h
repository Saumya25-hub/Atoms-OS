/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_CPP_SYSTEM_HANDLE_H_
#define MOJO_PUBLIC_CPP_SYSTEM_HANDLE_H_

#include "mojo/public/c/system/types.h"
#include "mojo/public/c/system/message_pipe.h"

namespace mojo {

class Handle {
public:
    Handle() : value_(MOJO_HANDLE_INVALID) {}
    explicit Handle(MojoHandle value) : value_(value) {}
    ~Handle() {}

    MojoHandle value() const { return value_; }
    void set_value(MojoHandle value) { value_ = value; }
    bool is_valid() const { return value_ != MOJO_HANDLE_INVALID; }
    void reset() { value_ = MOJO_HANDLE_INVALID; }

    bool operator==(const Handle& other) const { return value_ == other.value_; }
    bool operator!=(const Handle& other) const { return value_ != other.value_; }

protected:
    MojoHandle value_;
};

template <typename HandleType>
class ScopedHandleBase {
public:
    ScopedHandleBase() : handle_() {}
    explicit ScopedHandleBase(HandleType handle) : handle_(handle) {}
    ~ScopedHandleBase() { reset(); }

    ScopedHandleBase(ScopedHandleBase&& other) : handle_(other.release()) {}
    ScopedHandleBase& operator=(ScopedHandleBase&& other) {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    ScopedHandleBase(const ScopedHandleBase&) = delete;
    ScopedHandleBase& operator=(const ScopedHandleBase&) = delete;

    const HandleType& get() const { return handle_; }
    bool is_valid() const { return handle_.is_valid(); }

    void reset(HandleType handle = HandleType()) {
        if (handle_.is_valid()) {
            MojoClose(handle_.value());
        }
        handle_ = handle;
    }

    HandleType release() {
        HandleType unused = handle_;
        handle_.reset();
        return unused;
    }

private:
    HandleType handle_;
};

using ScopedHandle = ScopedHandleBase<Handle>;

} // namespace mojo

#endif // MOJO_PUBLIC_CPP_SYSTEM_HANDLE_H_
