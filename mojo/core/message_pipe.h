/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_CORE_MESSAGE_PIPE_H_
#define MOJO_CORE_MESSAGE_PIPE_H_

#include "mojo/public/c/system/types.h"
#include "userspace/runtime/cpp/include/vector"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

namespace mojo {
namespace core {

class Dispatcher {
public:
    virtual ~Dispatcher() {}
    virtual MojoHandleType GetType() const = 0;
    virtual void Close() = 0;
};

#define MOJO_MAX_MESSAGE_PAYLOAD (1024 * 1024) // 1MB Max
#define MOJO_MAX_MESSAGE_HANDLES 16
#define MOJO_MAX_QUEUE_DEPTH 256

struct RawMessage {
    std::vector<uint8_t> data;
    std::vector<MojoHandle> handles;
};

class MessagePipeDispatcher : public Dispatcher {
public:
    MessagePipeDispatcher(uint32_t owner_pid);
    virtual ~MessagePipeDispatcher();

    MojoHandleType GetType() const override { return MOJO_HANDLE_TYPE_MESSAGE_PIPE; }
    void Close() override;

    static void CreatePair(uint32_t pid_0, uint32_t pid_1, MessagePipeDispatcher** out_0, MessagePipeDispatcher** out_1);

    MojoResult WriteMessage(const void* bytes, uint32_t num_bytes, const MojoHandle* handles, uint32_t num_handles);
    MojoResult ReadMessage(void* out_bytes, uint32_t* inout_num_bytes, MojoHandle* out_handles, uint32_t* inout_num_handles);
    MojoHandleSignals QuerySignals() const;

    bool is_closed() const { return is_closed_; }
    bool is_peer_closed() const { return is_peer_closed_; }
    uint32_t owner_pid() const { return owner_pid_; }
    void set_owner_pid(uint32_t pid) { owner_pid_ = pid; }
    size_t queue_size() const { return incoming_queue_.size(); }

    void SetPeer(MessagePipeDispatcher* peer);
    void EnqueueMessageFromPeer(const RawMessage& msg);
    void NotifyPeerClosed();

private:
    MessagePipeDispatcher* peer_;
    std::vector<RawMessage> incoming_queue_;
    uint32_t owner_pid_;
    bool is_closed_;
    bool is_peer_closed_;
};

} // namespace core
} // namespace mojo

#endif // MOJO_CORE_MESSAGE_PIPE_H_
