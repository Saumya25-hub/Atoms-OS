/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "command_buffer.h"

namespace gpu {

CommandBuffer::CommandBuffer(size_t capacity)
    : capacity_(capacity) {
}

CommandBuffer::~CommandBuffer() {
    Clear();
}

bool CommandBuffer::Put(const GpuCommand& cmd) {
    if (queue_.size() >= capacity_) {
        return false; // Queue full
    }
    queue_.push_back(cmd);
    return true;
}

bool CommandBuffer::Get(GpuCommand* out_cmd) {
    if (!out_cmd || queue_.empty()) {
        return false;
    }
    *out_cmd = queue_[0];
    queue_.erase(0);
    return true;
}

void CommandBuffer::Clear() {
    queue_.clear();
}

} // namespace gpu
