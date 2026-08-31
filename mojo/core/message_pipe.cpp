/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "message_pipe.h"
#include "handle_table.h"
#include "userspace/runtime/c/include/string.h"

namespace mojo {
namespace core {

MessagePipeDispatcher::MessagePipeDispatcher(uint32_t owner_pid)
    : peer_(nullptr),
      owner_pid_(owner_pid),
      is_closed_(false),
      is_peer_closed_(false) {}

MessagePipeDispatcher::~MessagePipeDispatcher() {
    Close();
}

void MessagePipeDispatcher::CreatePair(uint32_t pid_0, uint32_t pid_1, MessagePipeDispatcher** out_0, MessagePipeDispatcher** out_1) {
    if (!out_0 || !out_1) return;

    MessagePipeDispatcher* d0 = new MessagePipeDispatcher(pid_0);
    MessagePipeDispatcher* d1 = new MessagePipeDispatcher(pid_1);

    d0->SetPeer(d1);
    d1->SetPeer(d0);

    *out_0 = d0;
    *out_1 = d1;
}

void MessagePipeDispatcher::SetPeer(MessagePipeDispatcher* peer) {
    peer_ = peer;
}

void MessagePipeDispatcher::Close() {
    if (is_closed_) return;
    is_closed_ = true;

    incoming_queue_.clear();

    if (peer_) {
        peer_->NotifyPeerClosed();
        peer_ = nullptr;
    }
}

void MessagePipeDispatcher::NotifyPeerClosed() {
    is_peer_closed_ = true;
    peer_ = nullptr;
}

MojoResult MessagePipeDispatcher::WriteMessage(const void* bytes, uint32_t num_bytes, const MojoHandle* handles, uint32_t num_handles) {
    if (is_closed_) return MOJO_RESULT_INVALID_ARGUMENT;
    if (is_peer_closed_ || !peer_) return MOJO_RESULT_FAILED_PRECONDITION;
    if (num_bytes > MOJO_MAX_MESSAGE_PAYLOAD) return MOJO_RESULT_RESOURCE_EXHAUSTED;
    if (num_handles > MOJO_MAX_MESSAGE_HANDLES) return MOJO_RESULT_RESOURCE_EXHAUSTED;

    if (peer_->queue_size() >= MOJO_MAX_QUEUE_DEPTH) {
        return MOJO_RESULT_SHOULD_WAIT;
    }

    RawMessage msg;
    if (bytes && num_bytes > 0) {
        msg.data.resize(num_bytes);
        memcpy(msg.data.data(), bytes, num_bytes);
    }

    if (handles && num_handles > 0) {
        HandleTable* ht = HandleTable::GetInstance();
        for (uint32_t i = 0; i < num_handles; i++) {
            MojoHandle transferred = MOJO_HANDLE_INVALID;
            MojoResult r = ht->TransferHandle(handles[i], peer_->owner_pid(), &transferred);
            if (r == MOJO_RESULT_OK) {
                msg.handles.push_back(transferred);
            }
        }
    }

    peer_->EnqueueMessageFromPeer(msg);
    return MOJO_RESULT_OK;
}

void MessagePipeDispatcher::EnqueueMessageFromPeer(const RawMessage& msg) {
    if (is_closed_) return;
    incoming_queue_.push_back(msg);
}

MojoResult MessagePipeDispatcher::ReadMessage(void* out_bytes, uint32_t* inout_num_bytes, MojoHandle* out_handles, uint32_t* inout_num_handles) {
    if (is_closed_) return MOJO_RESULT_INVALID_ARGUMENT;

    if (incoming_queue_.empty()) {
        if (is_peer_closed_) return MOJO_RESULT_FAILED_PRECONDITION;
        return MOJO_RESULT_SHOULD_WAIT;
    }

    const RawMessage& front = incoming_queue_[0];

    uint32_t required_bytes = (uint32_t)front.data.size();
    uint32_t required_handles = (uint32_t)front.handles.size();

    uint32_t max_bytes = inout_num_bytes ? *inout_num_bytes : 0;
    uint32_t max_handles = inout_num_handles ? *inout_num_handles : 0;

    if (max_bytes < required_bytes || max_handles < required_handles) {
        if (inout_num_bytes) *inout_num_bytes = required_bytes;
        if (inout_num_handles) *inout_num_handles = required_handles;
        return MOJO_RESULT_RESOURCE_EXHAUSTED;
    }

    if (out_bytes && required_bytes > 0) {
        memcpy(out_bytes, front.data.data(), required_bytes);
    }
    if (inout_num_bytes) *inout_num_bytes = required_bytes;

    if (out_handles && required_handles > 0) {
        for (uint32_t i = 0; i < required_handles; i++) {
            out_handles[i] = front.handles[i];
        }
    }
    if (inout_num_handles) *inout_num_handles = required_handles;

    incoming_queue_.erase(incoming_queue_.begin());
    return MOJO_RESULT_OK;
}

MojoHandleSignals MessagePipeDispatcher::QuerySignals() const {
    if (is_closed_) return MOJO_HANDLE_SIGNAL_NONE;

    MojoHandleSignals sigs = MOJO_HANDLE_SIGNAL_NONE;
    if (!incoming_queue_.empty()) {
        sigs |= MOJO_HANDLE_SIGNAL_READABLE;
    }
    if (!is_peer_closed_ && peer_) {
        sigs |= MOJO_HANDLE_SIGNAL_WRITABLE;
    }
    if (is_peer_closed_) {
        sigs |= MOJO_HANDLE_SIGNAL_PEER_CLOSED;
    }
    return sigs;
}

} // namespace core
} // namespace mojo
