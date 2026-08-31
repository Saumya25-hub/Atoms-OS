/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_GPU_COMMAND_BUFFER_COMMAND_BUFFER_H_
#define THIRD_PARTY_CHROMIUM_GPU_COMMAND_BUFFER_COMMAND_BUFFER_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "userspace/runtime/cpp/include/vector"

namespace gpu {

enum CommandType {
    CMD_NOP = 0,
    CMD_INITIALIZE = 1,
    CMD_CLEAR_COLOR = 2,
    CMD_CLEAR = 3,
    CMD_VIEWPORT = 4,
    CMD_CREATE_BUFFER = 5,
    CMD_BUFFER_DATA = 6,
    CMD_CREATE_TEXTURE = 7,
    CMD_TEX_IMAGE_2D = 8,
    CMD_CREATE_FRAMEBUFFER = 9,
    CMD_FRAMEBUFFER_TEXTURE_2D = 10,
    CMD_DRAW_ARRAYS = 11,
    CMD_DRAW_ELEMENTS = 12,
    CMD_SWAP_BUFFERS = 13,
    CMD_SHUTDOWN = 14
};

struct GpuCommand {
    CommandType type;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    float f1;
    float f2;
    float f3;
    float f4;
    const void* data_ptr;
    size_t data_size;
};

class CommandBuffer {
public:
    CommandBuffer(size_t capacity = 1024);
    ~CommandBuffer();

    bool Put(const GpuCommand& cmd);
    bool Get(GpuCommand* out_cmd);
    bool IsEmpty() const { return queue_.empty(); }
    size_t Count() const { return queue_.size(); }
    void Clear();

private:
    std::vector<GpuCommand> queue_;
    size_t capacity_;
};

} // namespace gpu

#endif // THIRD_PARTY_CHROMIUM_GPU_COMMAND_BUFFER_COMMAND_BUFFER_H_
