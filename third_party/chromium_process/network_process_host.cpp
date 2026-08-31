/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "network_process_host.h"
#include "kernel/browser_engine/process/abe_process.h"

namespace process {

NetworkProcessHost::NetworkProcessHost(uint32_t browser_pid)
    : pid_(0),
      cr3_(0),
      browser_pid_(browser_pid),
      url_loader_(&cookie_store_, &http_cache_),
      channel_(nullptr),
      browser_side_channel_(nullptr) {}

NetworkProcessHost::~NetworkProcessHost() {
    Terminate();
}

NetworkProcessHost* NetworkProcessHost::Create(uint32_t browser_pid) {
    NetworkProcessHost* host = new NetworkProcessHost(browser_pid);
    if (!host->Launch()) {
        delete host;
        return nullptr;
    }
    return host;
}

bool NetworkProcessHost::Launch() {
    if (pid_ != 0) return true;

    // 1. Instantiate dedicated network process with isolated address space
    uint32_t child_pid = 0;
    ABE_Error err = ABE_Process_Create(ABE_PROC_ROLE_NETWORKING, &child_pid);
    if (err != ABE_SUCCESS || child_pid == 0) {
        return false;
    }

    pid_ = child_pid;
    cr3_ = ABE_Process_GetCR3(pid_);

    // 2. Establish IPC channels
    channel_ = ipc::AtomsIPCChannel::Create("network-channel", pid_, browser_pid_);
    browser_side_channel_ = ipc::AtomsIPCChannel::Create("browser-net-channel", browser_pid_, pid_);

    if (channel_ && browser_side_channel_) {
        channel_->SetPeer(browser_side_channel_);
        browser_side_channel_->SetPeer(channel_);
    }

    return true;
}

net::URLLoaderResult NetworkProcessHost::Fetch(const std::string& url_str) {
    net::GURL gurl(url_str);
    if (!gurl.is_valid()) {
        net::URLLoaderResult bad;
        bad.net_error = net::ERR_INVALID_URL;
        return bad;
    }

    // Execute URL fetch through isolated URLLoader
    return url_loader_.Load(gurl);
}

void NetworkProcessHost::Terminate() {
    if (channel_) {
        channel_->Close();
        delete channel_;
        channel_ = nullptr;
    }
    if (browser_side_channel_) {
        browser_side_channel_->Close();
        delete browser_side_channel_;
        browser_side_channel_ = nullptr;
    }
    if (pid_ != 0) {
        ABE_Process_Terminate(pid_);
        pid_ = 0;
    }
}

} // namespace process
