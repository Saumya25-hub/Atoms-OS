/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_CPP_BINDINGS_PENDING_RECEIVER_H_
#define MOJO_PUBLIC_CPP_BINDINGS_PENDING_RECEIVER_H_

#include "mojo/public/cpp/system/message_pipe.h"

namespace mojo {

template <typename Interface>
class PendingReceiver {
public:
    PendingReceiver() : pipe_() {}
    explicit PendingReceiver(ScopedMessagePipeHandle pipe) : pipe_(std::move(pipe)) {}
    ~PendingReceiver() {}

    PendingReceiver(PendingReceiver&& other) : pipe_(std::move(other.pipe_)) {}
    PendingReceiver& operator=(PendingReceiver&& other) {
        pipe_ = std::move(other.pipe_);
        return *this;
    }

    PendingReceiver(const PendingReceiver&) = delete;
    PendingReceiver& operator=(const PendingReceiver&) = delete;

    bool is_valid() const { return pipe_.is_valid(); }
    ScopedMessagePipeHandle PassPipe() { return std::move(pipe_); }
    void reset() { pipe_.reset(); }

private:
    ScopedMessagePipeHandle pipe_;
};

} // namespace mojo

#endif // MOJO_PUBLIC_CPP_BINDINGS_PENDING_RECEIVER_H_
