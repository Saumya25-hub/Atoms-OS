/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_CPP_SYSTEM_MESSAGE_H_
#define MOJO_PUBLIC_CPP_SYSTEM_MESSAGE_H_

#include "mojo/public/c/system/types.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

namespace mojo {

enum MessageFlags {
    MESSAGE_FLAG_NONE             = 0,
    MESSAGE_FLAG_EXPECTS_RESPONSE = (1 << 0),
    MESSAGE_FLAG_IS_RESPONSE      = (1 << 1),
    MESSAGE_FLAG_SYNC             = (1 << 2)
};

struct MessageHeader {
    uint32_t total_size;
    uint32_t interface_id;
    uint32_t method_ordinal;
    uint32_t flags;
    uint32_t request_id;
    uint32_t payload_size;
};

#define MOJO_MESSAGE_MAX_STRING_LEN (64 * 1024)

class Message {
public:
    Message();
    Message(uint32_t interface_id, uint32_t method_ordinal, uint32_t flags = MESSAGE_FLAG_NONE, uint32_t request_id = 0);
    ~Message();

    // Serializer API
    void WriteInt32(int32_t val);
    void WriteUInt32(uint32_t val);
    void WriteInt64(int64_t val);
    void WriteUInt64(uint64_t val);
    void WriteBool(bool val);
    void WriteString(const std::string& str);
    void WriteBytes(const void* data, size_t size);
    void AttachHandle(MojoHandle handle);

    // Deserializer API
    bool ReadInt32(int32_t* out_val);
    bool ReadUInt32(uint32_t* out_val);
    bool ReadInt64(int64_t* out_val);
    bool ReadUInt64(uint64_t* out_val);
    bool ReadBool(bool* out_val);
    bool ReadString(std::string* out_str);
    bool ReadBytes(void* out_data, size_t size);
    bool ExtractHandle(MojoHandle* out_handle);

    // Header inspection
    uint32_t interface_id() const { return header_.interface_id; }
    uint32_t method_ordinal() const { return header_.method_ordinal; }
    uint32_t flags() const { return header_.flags; }
    uint32_t request_id() const { return header_.request_id; }
    uint32_t payload_size() const { return header_.payload_size; }
    size_t handle_count() const { return attached_handles_.size(); }
    const std::vector<MojoHandle>& handles() const { return attached_handles_; }

    // Buffer preparation & decoding
    std::vector<uint8_t> Serialize();
    bool Deserialize(const void* buffer, size_t size, const MojoHandle* handles, size_t handle_count);

    // Validation
    bool Validate() const;

private:
    MessageHeader header_;
    std::vector<uint8_t> payload_buffer_;
    std::vector<MojoHandle> attached_handles_;
    size_t read_offset_;
    size_t handle_read_offset_;
    bool is_valid_;
};

// High-level helper functions for message pipes
MojoResult WriteMessage(const MessagePipeHandle& handle, Message* message);
MojoResult ReadMessage(const MessagePipeHandle& handle, Message* out_message);

} // namespace mojo

#endif // MOJO_PUBLIC_CPP_SYSTEM_MESSAGE_H_
