/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_PROCESS_BROWSER_PROCESS_HOST_H_
#define THIRD_PARTY_CHROMIUM_PROCESS_BROWSER_PROCESS_HOST_H_

#include "renderer_process_host.h"
#include "network_process_host.h"
#include "utility_process_host.h"
#include "gpu_process_host.h"
#include "userspace/runtime/cpp/include/vector"
#include <stdint.h>
#include <stdbool.h>

namespace process {

class BrowserProcessHost {
public:
    BrowserProcessHost();
    ~BrowserProcessHost();

    static BrowserProcessHost* GetInstance();

    bool Initialize();
    void Shutdown();

    uint32_t browser_pid() const { return browser_pid_; }
    uint64_t browser_cr3() const { return browser_cr3_; }

    RendererProcessHost* CreateRendererHost();
    NetworkProcessHost* GetNetworkHost();
    UtilityProcessHost* GetUtilityHost();
    GpuProcessHost* GetGpuHost();

    RendererProcessHost* GetRendererByPID(uint32_t pid);
    size_t renderer_count() const { return renderers_.size(); }

    void OnRendererCrashed(uint32_t pid);
    bool ReloadRenderer(RendererProcessHost* renderer);
    void RemoveRenderer(RendererProcessHost* renderer);

private:
    uint32_t browser_pid_;
    uint64_t browser_cr3_;
    bool is_initialized_;
    std::vector<RendererProcessHost*> renderers_;
    NetworkProcessHost* network_host_;
    UtilityProcessHost* utility_host_;
    GpuProcessHost* gpu_host_;
};

} // namespace process

#endif // THIRD_PARTY_CHROMIUM_PROCESS_BROWSER_PROCESS_HOST_H_
