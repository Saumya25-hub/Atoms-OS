/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_IPC_ATOMS_IPC_CHANNEL_H_
#define THIRD_PARTY_CHROMIUM_IPC_ATOMS_IPC_CHANNEL_H_

#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

namespace ipc {

enum MessageType {
    MSG_HEARTBEAT_PING     = 0x0001,
    MSG_HEARTBEAT_PONG     = 0x0002,
    MSG_PROCESS_CRASH      = 0x000F,
    MSG_NAVIGATE           = 0x0010,  // Browser -> Renderer: URL / HTML payload
    MSG_RENDER_FRAME_READY = 0x0011,  // Renderer -> Browser: Frame painted in SHM surface
    MSG_DOM_EVENT          = 0x0012,  // Browser -> Renderer: User input event
    MSG_FETCH_REQUEST      = 0x0020,  // Renderer -> Network: HTTP URL fetch
    MSG_FETCH_RESPONSE     = 0x0021,  // Network -> Renderer: HTTP Response payload
    MSG_STORAGE_SET        = 0x0030,  // Renderer -> Utility: Store key-value
    MSG_STORAGE_GET        = 0x0031,  // Renderer -> Utility: Get key-value
    MSG_STORAGE_RESPONSE   = 0x0032,  // Utility -> Renderer: Storage value
};

#define IPC_MAX_PAYLOAD 1024

struct IPCMessage {
    uint32_t message_id;
    uint32_t type;
    uint32_t src_pid;
    uint32_t dst_pid;
    uint32_t payload_size;
    uint8_t  payload[IPC_MAX_PAYLOAD];
};

/*
 * AtomsIPCChannel implements a typed, bidirectional message passing channel
 * between distinct browser processes (e.g. Browser <-> Renderer, Renderer <-> Network).
 *
 * Designed as the Phase 13 IPC transport foundation, cleanly replaceable
 * by Chromium Mojo in Phase 14.
 */
class AtomsIPCChannel {
public:
    AtomsIPCChannel();
    AtomsIPCChannel(const std::string& name, uint32_t src_pid, uint32_t dst_pid);
    ~AtomsIPCChannel();

    static AtomsIPCChannel* Create(const std::string& name, uint32_t src_pid, uint32_t dst_pid);

    const std::string& name() const { return name_; }
    uint32_t src_pid() const { return src_pid_; }
    uint32_t dst_pid() const { return dst_pid_; }
    bool is_connected() const { return is_connected_; }

    bool Send(uint32_t type, const void* data, size_t size);
    bool SendString(uint32_t type, const std::string& text);
    bool Receive(IPCMessage* out_msg, bool non_blocking);
    void Close();

    size_t pending_messages_count() const { return incoming_queue_.size(); }

    // Direct cross-channel delivery mechanism for in-kernel / user task message routing
    void DeliverMessage(const IPCMessage& msg);

private:
    std::string name_;
    uint32_t src_pid_;
    uint32_t dst_pid_;
    bool is_connected_;
    uint32_t next_seq_;
    std::vector<IPCMessage> incoming_queue_;
    AtomsIPCChannel* peer_channel_;

public:
    void SetPeer(AtomsIPCChannel* peer) { peer_channel_ = peer; }
};

} // namespace ipc

#endif // THIRD_PARTY_CHROMIUM_IPC_ATOMS_IPC_CHANNEL_H_
