/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_CPP_SYSTEM_MESSAGE_PIPE_H_
#define MOJO_PUBLIC_CPP_SYSTEM_MESSAGE_PIPE_H_

#include "handle.h"
#include "mojo/public/c/system/message_pipe.h"

namespace mojo {

class MessagePipeHandle : public Handle {
public:
    MessagePipeHandle() : Handle() {}
    explicit MessagePipeHandle(MojoHandle value) : Handle(value) {}

    MojoResult WriteMessage(const void* bytes, uint32_t num_bytes, const MojoHandle* handles = nullptr, uint32_t num_handles = 0) const {
        return MojoWriteMessage(value_, bytes, num_bytes, handles, num_handles, nullptr);
    }

    MojoResult ReadMessage(void* out_bytes, uint32_t* inout_num_bytes, MojoHandle* out_handles = nullptr, uint32_t* inout_num_handles = nullptr) const {
        return MojoReadMessage(value_, out_bytes, inout_num_bytes, out_handles, inout_num_handles, nullptr);
    }

    MojoHandleSignals QuerySignals() const {
        MojoHandleSignals sigs = MOJO_HANDLE_SIGNAL_NONE;
        MojoQueryHandleSignals(value_, &sigs);
        return sigs;
    }
};

using ScopedMessagePipeHandle = ScopedHandleBase<MessagePipeHandle>;

inline MojoResult CreateMessagePipe(const MojoCreateMessagePipeOptions* options,
                                   ScopedMessagePipeHandle* out_handle_0,
                                   ScopedMessagePipeHandle* out_handle_1) {
    MojoHandle h0 = MOJO_HANDLE_INVALID;
    MojoHandle h1 = MOJO_HANDLE_INVALID;
    MojoResult result = MojoCreateMessagePipe(options, &h0, &h1);
    if (result == MOJO_RESULT_OK) {
        out_handle_0->reset(MessagePipeHandle(h0));
        out_handle_1->reset(MessagePipeHandle(h1));
    }
    return result;
}

} // namespace mojo

#endif // MOJO_PUBLIC_CPP_SYSTEM_MESSAGE_PIPE_H_
