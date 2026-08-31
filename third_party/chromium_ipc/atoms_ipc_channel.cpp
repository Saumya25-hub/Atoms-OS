/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "atoms_ipc_channel.h"
#include "userspace/runtime/c/include/string.h"

namespace ipc {

AtomsIPCChannel::AtomsIPCChannel()
    : name_(""),
      src_pid_(0),
      dst_pid_(0),
      is_connected_(false),
      next_seq_(1),
      peer_channel_(nullptr) {}

AtomsIPCChannel::AtomsIPCChannel(const std::string& name, uint32_t src_pid, uint32_t dst_pid)
    : name_(name),
      src_pid_(src_pid),
      dst_pid_(dst_pid),
      is_connected_(true),
      next_seq_(1),
      peer_channel_(nullptr) {}

AtomsIPCChannel::~AtomsIPCChannel() {
    Close();
}

AtomsIPCChannel* AtomsIPCChannel::Create(const std::string& name, uint32_t src_pid, uint32_t dst_pid) {
    return new AtomsIPCChannel(name, src_pid, dst_pid);
}

bool AtomsIPCChannel::Send(uint32_t type, const void* data, size_t size) {
    if (!is_connected_) return false;
    if (size > IPC_MAX_PAYLOAD) return false;

    IPCMessage msg;
    msg.message_id = next_seq_++;
    msg.type = type;
    msg.src_pid = src_pid_;
    msg.dst_pid = dst_pid_;
    msg.payload_size = (uint32_t)size;
    memset(msg.payload, 0, sizeof(msg.payload));

    if (data && size > 0) {
        memcpy(msg.payload, data, size);
    }

    // Deliver to peer if linked
    if (peer_channel_ && peer_channel_->is_connected()) {
        peer_channel_->DeliverMessage(msg);
        return true;
    }

    return true;
}

bool AtomsIPCChannel::SendString(uint32_t type, const std::string& text) {
    return Send(type, text.c_str(), text.size() + 1);
}

void AtomsIPCChannel::DeliverMessage(const IPCMessage& msg) {
    if (!is_connected_) return;
    incoming_queue_.push_back(msg);
}

bool AtomsIPCChannel::Receive(IPCMessage* out_msg, bool non_blocking) {
    (void)non_blocking;
    if (!out_msg) return false;
    if (incoming_queue_.empty()) return false;

    *out_msg = incoming_queue_[0];
    incoming_queue_.erase(incoming_queue_.begin());
    return true;
}

void AtomsIPCChannel::Close() {
    is_connected_ = false;
    incoming_queue_.clear();
    if (peer_channel_) {
        peer_channel_->peer_channel_ = nullptr;
        peer_channel_ = nullptr;
    }
}

} // namespace ipc
