/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "utility_process_host.h"
#include "kernel/browser_engine/process/abe_process.h"

namespace process {

UtilityProcessHost::UtilityProcessHost(uint32_t browser_pid)
    : pid_(0),
      cr3_(0),
      browser_pid_(browser_pid),
      channel_(nullptr),
      browser_side_channel_(nullptr) {}

UtilityProcessHost::~UtilityProcessHost() {
    Terminate();
}

UtilityProcessHost* UtilityProcessHost::Create(uint32_t browser_pid) {
    UtilityProcessHost* host = new UtilityProcessHost(browser_pid);
    if (!host->Launch()) {
        delete host;
        return nullptr;
    }
    return host;
}

bool UtilityProcessHost::Launch() {
    if (pid_ != 0) return true;

    // 1. Instantiate dedicated utility process with isolated address space
    uint32_t child_pid = 0;
    ABE_Error err = ABE_Process_Create(ABE_PROC_ROLE_UTILITY, &child_pid);
    if (err != ABE_SUCCESS || child_pid == 0) {
        return false;
    }

    pid_ = child_pid;
    cr3_ = ABE_Process_GetCR3(pid_);

    // 2. Establish IPC channels
    channel_ = ipc::AtomsIPCChannel::Create("utility-channel", pid_, browser_pid_);
    browser_side_channel_ = ipc::AtomsIPCChannel::Create("browser-util-channel", browser_pid_, pid_);

    if (channel_ && browser_side_channel_) {
        channel_->SetPeer(browser_side_channel_);
        browser_side_channel_->SetPeer(channel_);
    }

    return true;
}

bool UtilityProcessHost::SetLocalStorageItem(const net::SecurityOrigin& origin, const std::string& key, const std::string& value) {
    storage::StorageArea* area = local_storage_manager_.GetLocalStorage(origin);
    if (!area) return false;
    return area->setItem(key, value);
}

std::string UtilityProcessHost::GetLocalStorageItem(const net::SecurityOrigin& origin, const std::string& key) {
    storage::StorageArea* area = local_storage_manager_.GetLocalStorage(origin);
    if (!area) return "";
    return area->getItem(key);
}

void UtilityProcessHost::Terminate() {
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
