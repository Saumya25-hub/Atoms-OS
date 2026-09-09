/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_CPP_BINDINGS_PENDING_REMOTE_H_
#define MOJO_PUBLIC_CPP_BINDINGS_PENDING_REMOTE_H_

#include "mojo/public/cpp/system/message_pipe.h"

namespace mojo {

template <typename Interface>
class PendingRemote {
public:
    PendingRemote() : pipe_() {}
    explicit PendingRemote(ScopedMessagePipeHandle pipe) : pipe_(std::move(pipe)) {}
    ~PendingRemote() {}

    PendingRemote(PendingRemote&& other) : pipe_(std::move(other.pipe_)) {}
    PendingRemote& operator=(PendingRemote&& other) {
        pipe_ = std::move(other.pipe_);
        return *this;
    }

    PendingRemote(const PendingRemote&) = delete;
    PendingRemote& operator=(const PendingRemote&) = delete;

    bool is_valid() const { return pipe_.is_valid(); }
    ScopedMessagePipeHandle PassPipe() { return std::move(pipe_); }
    void reset() { pipe_.reset(); }

private:
    ScopedMessagePipeHandle pipe_;
};

} // namespace mojo

#endif // MOJO_PUBLIC_CPP_BINDINGS_PENDING_REMOTE_H_
