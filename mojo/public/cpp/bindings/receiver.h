/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_CPP_BINDINGS_RECEIVER_H_
#define MOJO_PUBLIC_CPP_BINDINGS_RECEIVER_H_

#include "pending_receiver.h"
#include "mojo/public/cpp/system/message.h"

namespace mojo {

typedef void (*DisconnectCallback)(void* context);

template <typename Interface>
class Receiver {
public:
    explicit Receiver(Interface* impl)
        : impl_(impl),
          pipe_(),
          disconnect_callback_(nullptr),
          disconnect_context_(nullptr) {}

    Receiver(Interface* impl, PendingReceiver<Interface> pending_receiver)
        : impl_(impl),
          pipe_(pending_receiver.PassPipe()),
          disconnect_callback_(nullptr),
          disconnect_context_(nullptr) {}

    ~Receiver() { reset(); }

    bool is_bound() const { return pipe_.is_valid(); }

    void Bind(PendingReceiver<Interface> pending_receiver) {
        reset();
        pipe_ = pending_receiver.PassPipe();
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

    Interface* impl() const { return impl_; }
    const ScopedMessagePipeHandle& handle() const { return pipe_; }

    // Drains any available message from the pipe
    bool PumpMessage(Message* out_msg) {
        if (!pipe_.is_valid()) return false;
        MojoResult res = ReadMessage(pipe_.get(), out_msg);
        if (res == MOJO_RESULT_FAILED_PRECONDITION) {
            // Peer closed
            reset();
            return false;
        }
        return (res == MOJO_RESULT_OK);
    }

private:
    Interface* impl_;
    ScopedMessagePipeHandle pipe_;
    DisconnectCallback disconnect_callback_;
    void* disconnect_context_;
};

} // namespace mojo

#endif // MOJO_PUBLIC_CPP_BINDINGS_RECEIVER_H_
