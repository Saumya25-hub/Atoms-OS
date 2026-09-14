/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "message.h"
#include "userspace/runtime/c/include/string.h"

namespace mojo {

Message::Message()
    : read_offset_(0),
      handle_read_offset_(0),
      is_valid_(true) {
    memset(&header_, 0, sizeof(header_));
}

Message::Message(uint32_t interface_id, uint32_t method_ordinal, uint32_t flags, uint32_t request_id)
    : read_offset_(0),
      handle_read_offset_(0),
      is_valid_(true) {
    header_.total_size = (uint32_t)sizeof(MessageHeader);
    header_.interface_id = interface_id;
    header_.method_ordinal = method_ordinal;
    header_.flags = flags;
    header_.request_id = request_id;
    header_.payload_size = 0;
}

Message::~Message() {}

void Message::WriteInt32(int32_t val) {
    WriteBytes(&val, sizeof(val));
}

void Message::WriteUInt32(uint32_t val) {
    WriteBytes(&val, sizeof(val));
}

void Message::WriteInt64(int64_t val) {
    WriteBytes(&val, sizeof(val));
}

void Message::WriteUInt64(uint64_t val) {
    WriteBytes(&val, sizeof(val));
}

void Message::WriteBool(bool val) {
    uint8_t b = val ? 1 : 0;
    WriteBytes(&b, sizeof(b));
}

void Message::WriteString(const std::string& str) {
    uint32_t len = (uint32_t)str.size();
    WriteUInt32(len);
    if (len > 0) {
        WriteBytes(str.c_str(), len);
    }
}

void Message::WriteBytes(const void* data, size_t size) {
    if (!data || size == 0) return;
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; i++) {
        payload_buffer_.push_back(ptr[i]);
    }
    header_.payload_size = (uint32_t)payload_buffer_.size();
    header_.total_size = (uint32_t)(sizeof(MessageHeader) + payload_buffer_.size());
}

void Message::AttachHandle(MojoHandle handle) {
    if (handle != MOJO_HANDLE_INVALID) {
        attached_handles_.push_back(handle);
    }
}

bool Message::ReadInt32(int32_t* out_val) {
    return ReadBytes(out_val, sizeof(int32_t));
}

bool Message::ReadUInt32(uint32_t* out_val) {
    return ReadBytes(out_val, sizeof(uint32_t));
}

bool Message::ReadInt64(int64_t* out_val) {
    return ReadBytes(out_val, sizeof(int64_t));
}

bool Message::ReadUInt64(uint64_t* out_val) {
    return ReadBytes(out_val, sizeof(uint64_t));
}

bool Message::ReadBool(bool* out_val) {
    uint8_t b = 0;
    if (!ReadBytes(&b, sizeof(b))) return false;
    if (out_val) *out_val = (b != 0);
    return true;
}

bool Message::ReadString(std::string* out_str) {
    uint32_t len = 0;
    if (!ReadUInt32(&len)) return false;
    if (len > MOJO_MESSAGE_MAX_STRING_LEN) return false;

    if (len == 0) {
        if (out_str) *out_str = "";
        return true;
    }

    if (read_offset_ + len > payload_buffer_.size()) {
        return false;
    }

    if (out_str) {
        *out_str = std::string(reinterpret_cast<const char*>(&payload_buffer_[read_offset_]), len);
    }
    read_offset_ += len;
    return true;
}

bool Message::ReadBytes(void* out_data, size_t size) {
    if (!out_data || size == 0) return false;
    if (read_offset_ + size > payload_buffer_.size()) {
        return false;
    }
    memcpy(out_data, &payload_buffer_[read_offset_], size);
    read_offset_ += size;
    return true;
}

bool Message::ExtractHandle(MojoHandle* out_handle) {
    if (!out_handle) return false;
    if (handle_read_offset_ >= attached_handles_.size()) {
        return false;
    }
    *out_handle = attached_handles_[handle_read_offset_++];
    return true;
}

std::vector<uint8_t> Message::Serialize() {
    header_.payload_size = (uint32_t)payload_buffer_.size();
    header_.total_size = (uint32_t)(sizeof(MessageHeader) + payload_buffer_.size());

    std::vector<uint8_t> out;
    out.resize(header_.total_size);

    memcpy(out.data(), &header_, sizeof(MessageHeader));
    if (!payload_buffer_.empty()) {
        memcpy(out.data() + sizeof(MessageHeader), payload_buffer_.data(), payload_buffer_.size());
    }

    return out;
}

bool Message::Deserialize(const void* buffer, size_t size, const MojoHandle* handles, size_t handle_count) {
    if (!buffer || size < sizeof(MessageHeader)) {
        is_valid_ = false;
        return false;
    }

    memcpy(&header_, buffer, sizeof(MessageHeader));

    if (header_.total_size != size || header_.payload_size != (size - sizeof(MessageHeader))) {
        is_valid_ = false;
        return false;
    }

    payload_buffer_.resize(header_.payload_size);
    if (header_.payload_size > 0) {
        memcpy(payload_buffer_.data(), static_cast<const uint8_t*>(buffer) + sizeof(MessageHeader), header_.payload_size);
    }

    attached_handles_.clear();
    if (handles && handle_count > 0) {
        for (size_t i = 0; i < handle_count; i++) {
            attached_handles_.push_back(handles[i]);
        }
    }

    read_offset_ = 0;
    handle_read_offset_ = 0;
    is_valid_ = true;
    return true;
}

bool Message::Validate() const {
    if (!is_valid_) return false;
    if (header_.total_size < sizeof(MessageHeader)) return false;
    if (header_.total_size > (1024 * 1024)) return false; // 1MB Cap
    if (header_.payload_size != payload_buffer_.size()) return false;
    return true;
}

MojoResult WriteMessage(const MessagePipeHandle& handle, Message* message) {
    if (!handle.is_valid() || !message || !message->Validate()) {
        return MOJO_RESULT_INVALID_ARGUMENT;
    }

    std::vector<uint8_t> bytes = message->Serialize();
    const std::vector<MojoHandle>& handles = message->handles();

    return handle.WriteMessage(
        bytes.data(),
        (uint32_t)bytes.size(),
        handles.empty() ? nullptr : handles.data(),
        (uint32_t)handles.size());
}

MojoResult ReadMessage(const MessagePipeHandle& handle, Message* out_message) {
    if (!handle.is_valid() || !out_message) {
        return MOJO_RESULT_INVALID_ARGUMENT;
    }

    uint8_t buffer[4096];
    uint32_t num_bytes = sizeof(buffer);
    MojoHandle handles[16];
    uint32_t num_handles = 16;

    MojoResult res = handle.ReadMessage(buffer, &num_bytes, handles, &num_handles);
    if (res != MOJO_RESULT_OK) {
        return res;
    }

    if (!out_message->Deserialize(buffer, num_bytes, handles, num_handles)) {
        return MOJO_RESULT_DATA_LOSS;
    }

    return MOJO_RESULT_OK;
}

} // namespace mojo
