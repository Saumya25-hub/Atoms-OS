/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gpu_process_host.h"
#include "kernel/core/memory/vmm/include/vmm.h"

static uint32_t g_next_gpu_pid = 701;

namespace process {

GpuProcessHost::GpuProcessHost(uint32_t browser_pid)
    : pid_(0),
      cr3_(0),
      browser_pid_(browser_pid),
      state_(GPU_PROCESS_UNINITIALIZED),
      channel_host_(nullptr) {
}

GpuProcessHost::~GpuProcessHost() {
    Terminate();
}

GpuProcessHost* GpuProcessHost::Create(uint32_t browser_pid) {
    GpuProcessHost* host = new GpuProcessHost(browser_pid);
    if (!host->Launch()) {
        delete host;
        return nullptr;
    }
    return host;
}

bool GpuProcessHost::Launch() {
    pid_ = g_next_gpu_pid++;
    cr3_ = 0x60000000ULL + (uint64_t)pid_ * 0x10000ULL;

    decoder_.Initialize(1, 1920, 1080);
    state_ = GPU_PROCESS_RUNNING;
    return true;
}

void GpuProcessHost::Terminate() {
    if (state_ == GPU_PROCESS_TERMINATED) return;

    decoder_.Shutdown();
    if (channel_host_) {
        channel_host_->Destroy();
        delete channel_host_;
        channel_host_ = nullptr;
    }

    state_ = GPU_PROCESS_TERMINATED;
}

void GpuProcessHost::SimulateCrash() {
    state_ = GPU_PROCESS_CRASHED;
    decoder_.Shutdown();
}

gpu::GpuChannelHost* GpuProcessHost::CreateChannelHost(uint32_t channel_id) {
    if (channel_host_) {
        delete channel_host_;
    }
    channel_host_ = new gpu::GpuChannelHost(channel_id);
    channel_host_->Initialize(1, 1920, 1080);
    return channel_host_;
}

} // namespace process
