/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_GPU_COMMAND_BUFFER_GPU_COMMAND_DECODER_H_
#define THIRD_PARTY_CHROMIUM_GPU_COMMAND_BUFFER_GPU_COMMAND_DECODER_H_

#include "command_buffer.h"
#include "kernel/graphics/gl/gl.h"
#include "kernel/graphics/bgl/bgl.h"

namespace gpu {

class GpuCommandDecoder {
public:
    GpuCommandDecoder();
    ~GpuCommandDecoder();

    bool Initialize(uint32_t window_id, uint32_t width, uint32_t height);
    void Shutdown();

    bool ProcessCommand(const GpuCommand& cmd);
    bool FlushCommands(CommandBuffer* buffer);

    bool is_initialized() const { return is_initialized_; }
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    uint32_t draw_call_count() const { return draw_call_count_; }
    uint32_t frame_count() const { return frame_count_; }

    BGLContext* context() const { return context_; }
    BGLDrawable* drawable() const { return drawable_; }

private:
    bool is_initialized_;
    uint32_t window_id_;
    uint32_t width_;
    uint32_t height_;
    uint32_t draw_call_count_;
    uint32_t frame_count_;
    BGLContext* context_;
    BGLDrawable* drawable_;
};

} // namespace gpu

#endif // THIRD_PARTY_CHROMIUM_GPU_COMMAND_BUFFER_GPU_COMMAND_DECODER_H_
