/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_PROCESS_UTILITY_PROCESS_HOST_H_
#define THIRD_PARTY_CHROMIUM_PROCESS_UTILITY_PROCESS_HOST_H_

#include "third_party/chromium_ipc/atoms_ipc_channel.h"
#include "third_party/chromium_storage/dom_storage/local_storage_manager.h"
#include "third_party/chromium_storage/dom_storage/session_storage_manager.h"
#include "userspace/runtime/cpp/include/string"
#include <stdint.h>
#include <stdbool.h>

namespace process {

class UtilityProcessHost {
public:
    UtilityProcessHost(uint32_t browser_pid);
    ~UtilityProcessHost();

    static UtilityProcessHost* Create(uint32_t browser_pid);

    bool Launch();
    bool SetLocalStorageItem(const net::SecurityOrigin& origin, const std::string& key, const std::string& value);
    std::string GetLocalStorageItem(const net::SecurityOrigin& origin, const std::string& key);
    void Terminate();

    uint32_t pid() const { return pid_; }
    uint64_t cr3() const { return cr3_; }
    bool is_alive() const { return pid_ != 0; }

    ipc::AtomsIPCChannel* channel() { return channel_; }

private:
    uint32_t pid_;
    uint64_t cr3_;
    uint32_t browser_pid_;
    storage::LocalStorageManager local_storage_manager_;
    storage::SessionStorageManager session_storage_manager_;
    ipc::AtomsIPCChannel* channel_;
    ipc::AtomsIPCChannel* browser_side_channel_;
};

} // namespace process

#endif // THIRD_PARTY_CHROMIUM_PROCESS_UTILITY_PROCESS_HOST_H_
