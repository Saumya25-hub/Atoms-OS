/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "browser_process_host.h"
#include "kernel/browser_engine/process/abe_process.h"
extern "C" {
#include "kernel/core/memory/vmm/include/vmm.h"
}

namespace process {

static BrowserProcessHost* s_instance = nullptr;

BrowserProcessHost::BrowserProcessHost()
    : browser_pid_(0),
      browser_cr3_(0),
      is_initialized_(false),
      network_host_(nullptr),
      utility_host_(nullptr),
      gpu_host_(nullptr) {}

BrowserProcessHost::~BrowserProcessHost() {
    Shutdown();
}

BrowserProcessHost* BrowserProcessHost::GetInstance() {
    if (!s_instance) {
        s_instance = new BrowserProcessHost();
        s_instance->Initialize();
    }
    return s_instance;
}

bool BrowserProcessHost::Initialize() {
    if (is_initialized_) return true;

    // 1. Initialize kernel browser process manager
    ABE_Process_Init();

    // 2. Identify main browser PID and CR3
    ABE_ProcessNode* main_node = ABE_Process_GetByIndex(0);
    if (main_node) {
        browser_pid_ = main_node->process_id;
        browser_cr3_ = main_node->pml4_phys;
    } else {
        browser_pid_ = 100;
        browser_cr3_ = (uint64_t)vmm_get_active_pml4();
    }

    is_initialized_ = true;
    return true;
}

RendererProcessHost* BrowserProcessHost::CreateRendererHost() {
    if (!is_initialized_) Initialize();

    RendererProcessHost* host = RendererProcessHost::Create(browser_pid_);
    if (host) {
        renderers_.push_back(host);
    }
    return host;
}

NetworkProcessHost* BrowserProcessHost::GetNetworkHost() {
    if (!is_initialized_) Initialize();

    if (!network_host_ || !network_host_->is_alive()) {
        if (network_host_) delete network_host_;
        network_host_ = NetworkProcessHost::Create(browser_pid_);
    }
    return network_host_;
}

UtilityProcessHost* BrowserProcessHost::GetUtilityHost() {
    if (!is_initialized_) Initialize();

    if (!utility_host_ || !utility_host_->is_alive()) {
        if (utility_host_) delete utility_host_;
        utility_host_ = UtilityProcessHost::Create(browser_pid_);
    }
    return utility_host_;
}

GpuProcessHost* BrowserProcessHost::GetGpuHost() {
    if (!is_initialized_) Initialize();

    if (!gpu_host_ || !gpu_host_->is_alive()) {
        if (gpu_host_) delete gpu_host_;
        gpu_host_ = GpuProcessHost::Create(browser_pid_);
    }
    return gpu_host_;
}

RendererProcessHost* BrowserProcessHost::GetRendererByPID(uint32_t pid) {
    for (size_t i = 0; i < renderers_.size(); i++) {
        if (renderers_[i]->pid() == pid) {
            return renderers_[i];
        }
    }
    return nullptr;
}

void BrowserProcessHost::OnRendererCrashed(uint32_t pid) {
    RendererProcessHost* host = GetRendererByPID(pid);
    if (host) {
        host->SimulateCrash();
    }
}

bool BrowserProcessHost::ReloadRenderer(RendererProcessHost* renderer) {
    if (!renderer) return false;

    std::string last_url = renderer->current_url();
    renderer->Terminate();

    // Spawn a fresh renderer process with a new PID and CR3
    bool ok = renderer->Launch();
    if (ok && !last_url.empty() && last_url != "about:blank") {
        renderer->Navigate(last_url, "<html><body><h1>Reloaded Tab</h1></body></html>");
    }
    return ok;
}

void BrowserProcessHost::RemoveRenderer(RendererProcessHost* renderer) {
    for (size_t i = 0; i < renderers_.size(); i++) {
        if (renderers_[i] == renderer) {
            renderers_.erase(renderers_.begin() + i);
            break;
        }
    }
}

void BrowserProcessHost::Shutdown() {
    for (size_t i = 0; i < renderers_.size(); i++) {
        renderers_[i]->Terminate();
        delete renderers_[i];
    }
    renderers_.clear();

    if (network_host_) {
        network_host_->Terminate();
        delete network_host_;
        network_host_ = nullptr;
    }

    if (utility_host_) {
        utility_host_->Terminate();
        delete utility_host_;
        utility_host_ = nullptr;
    }

    ABE_Process_Shutdown();
    is_initialized_ = false;
}

} // namespace process
