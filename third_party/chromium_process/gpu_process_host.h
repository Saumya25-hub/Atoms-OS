/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_PROCESS_GPU_PROCESS_HOST_H_
#define THIRD_PARTY_CHROMIUM_PROCESS_GPU_PROCESS_HOST_H_

#include "third_party/chromium_gpu/command_buffer/gpu_command_decoder.h"
#include "third_party/chromium_gpu/command_buffer/gpu_channel_host.h"
#include <stdint.h>
#include <stdbool.h>

namespace process {

enum GpuProcessState {
    GPU_PROCESS_UNINITIALIZED = 0,
    GPU_PROCESS_RUNNING = 1,
    GPU_PROCESS_CRASHED = 2,
    GPU_PROCESS_TERMINATED = 3
};

class GpuProcessHost {
public:
    GpuProcessHost(uint32_t browser_pid);
    ~GpuProcessHost();

    static GpuProcessHost* Create(uint32_t browser_pid);

    bool Launch();
    void Terminate();
    void SimulateCrash();

    uint32_t pid() const { return pid_; }
    uint64_t cr3() const { return cr3_; }
    uint32_t browser_pid() const { return browser_pid_; }
    GpuProcessState state() const { return state_; }
    bool is_alive() const { return state_ == GPU_PROCESS_RUNNING; }

    gpu::GpuCommandDecoder* decoder() { return &decoder_; }
    gpu::GpuChannelHost* CreateChannelHost(uint32_t channel_id = 1);

private:
    uint32_t pid_;
    uint64_t cr3_;
    uint32_t browser_pid_;
    GpuProcessState state_;
    gpu::GpuCommandDecoder decoder_;
    gpu::GpuChannelHost* channel_host_;
};

} // namespace process

#endif // THIRD_PARTY_CHROMIUM_PROCESS_GPU_PROCESS_HOST_H_
