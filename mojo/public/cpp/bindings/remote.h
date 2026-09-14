/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_CPP_BINDINGS_REMOTE_H_
#define MOJO_PUBLIC_CPP_BINDINGS_REMOTE_H_

#include "pending_remote.h"
#include "mojo/public/cpp/system/message.h"

namespace mojo {

template <typename Interface>
class Remote {
public:
    Remote()
        : pipe_(),
          disconnect_callback_(nullptr),
          disconnect_context_(nullptr) {}

    explicit Remote(PendingRemote<Interface> pending_remote)
        : pipe_(pending_remote.PassPipe()),
          disconnect_callback_(nullptr),
          disconnect_context_(nullptr) {}

    ~Remote() { reset(); }

    bool is_bound() const { return pipe_.is_valid(); }
    bool is_connected() const {
        if (!pipe_.is_valid()) return false;
        MojoHandleSignals sigs = pipe_.get().QuerySignals();
        return !(sigs & MOJO_HANDLE_SIGNAL_PEER_CLOSED);
    }

    void Bind(PendingRemote<Interface> pending_remote) {
        reset();
        pipe_ = pending_remote.PassPipe();
    }

    void reset() {
        if (pipe_.is_valid()) {
            pipe_.reset();
            if (disconnect_callback_) {
                disconnect_callback_(disconnect_context_);
            }
        }
    }

    void set_disconnect_handler(DisconnectCallback cb, void* context = nullptr) {
        disconnect_callback_ = cb;
        disconnect_context_ = context;
    }

    ScopedMessagePipeHandle Unbind() {
        return std::move(pipe_);
    }

    const ScopedMessagePipeHandle& handle() const { return pipe_; }

    MojoResult SendMessage(Message* message) {
        if (!pipe_.is_valid()) return MOJO_RESULT_INVALID_ARGUMENT;
        MojoResult res = WriteMessage(pipe_.get(), message);
        if (res == MOJO_RESULT_FAILED_PRECONDITION) {
            reset();
        }
        return res;
    }

    Interface* get() {
        return static_cast<Interface*>(this);
    }

    Interface* operator->() {
        return get();
    }

private:
    ScopedMessagePipeHandle pipe_;
    DisconnectCallback disconnect_callback_;
    void* disconnect_context_;
};

} // namespace mojo

#endif // MOJO_PUBLIC_CPP_BINDINGS_REMOTE_H_
