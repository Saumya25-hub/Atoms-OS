/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gpu_channel_host.h"

namespace gpu {

GpuChannelHost::GpuChannelHost(uint32_t channel_id)
    : channel_id_(channel_id),
      is_connected_(false),
      command_buffer_(1024) {
}

GpuChannelHost::~GpuChannelHost() {
    Destroy();
}

bool GpuChannelHost::Initialize(uint32_t window_id, uint32_t width, uint32_t height) {
    if (is_connected_) return true;

    bool ok = decoder_.Initialize(window_id, width, height);
    if (!ok) return false;

    is_connected_ = true;
    return true;
}

void GpuChannelHost::Destroy() {
    if (!is_connected_) return;

    decoder_.Shutdown();
    command_buffer_.Clear();
    is_connected_ = false;
}

bool GpuChannelHost::SendCommand(const GpuCommand& cmd) {
    if (!is_connected_) return false;
    return command_buffer_.Put(cmd);
}

bool GpuChannelHost::Flush() {
    if (!is_connected_) return false;
    return decoder_.FlushCommands(&command_buffer_);
}

} // namespace gpu
