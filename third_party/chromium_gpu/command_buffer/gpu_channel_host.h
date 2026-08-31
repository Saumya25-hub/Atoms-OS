/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_GPU_COMMAND_BUFFER_GPU_CHANNEL_HOST_H_
#define THIRD_PARTY_CHROMIUM_GPU_COMMAND_BUFFER_GPU_CHANNEL_HOST_H_

#include "command_buffer.h"
#include "gpu_command_decoder.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace gpu {

class GpuChannelHost {
public:
    GpuChannelHost(uint32_t channel_id = 1);
    ~GpuChannelHost();

    bool Initialize(uint32_t window_id, uint32_t width, uint32_t height);
    void Destroy();

    bool SendCommand(const GpuCommand& cmd);
    bool Flush();

    uint32_t channel_id() const { return channel_id_; }
    bool is_connected() const { return is_connected_; }
    CommandBuffer* command_buffer() { return &command_buffer_; }
    GpuCommandDecoder* decoder() { return &decoder_; }

private:
    uint32_t channel_id_;
    bool is_connected_;
    CommandBuffer command_buffer_;
    GpuCommandDecoder decoder_;
    mojo::ScopedMessagePipeHandle pipe_;
};

} // namespace gpu

#endif // THIRD_PARTY_CHROMIUM_GPU_COMMAND_BUFFER_GPU_CHANNEL_HOST_H_
