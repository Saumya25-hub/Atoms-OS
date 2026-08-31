/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_PROCESS_NETWORK_PROCESS_HOST_H_
#define THIRD_PARTY_CHROMIUM_PROCESS_NETWORK_PROCESS_HOST_H_

#include "third_party/chromium_ipc/atoms_ipc_channel.h"
#include "third_party/chromium_net/url_request/url_loader.h"
#include "userspace/runtime/cpp/include/string"
#include <stdint.h>
#include <stdbool.h>

namespace process {

class NetworkProcessHost {
public:
    NetworkProcessHost(uint32_t browser_pid);
    ~NetworkProcessHost();

    static NetworkProcessHost* Create(uint32_t browser_pid);

    bool Launch();
    net::URLLoaderResult Fetch(const std::string& url);
    void Terminate();

    uint32_t pid() const { return pid_; }
    uint64_t cr3() const { return cr3_; }
    bool is_alive() const { return pid_ != 0; }

    ipc::AtomsIPCChannel* channel() { return channel_; }

private:
    uint32_t pid_;
    uint64_t cr3_;
    uint32_t browser_pid_;
    net::CookieStore cookie_store_;
    net::HttpCache http_cache_;
    net::URLLoader url_loader_;
    ipc::AtomsIPCChannel* channel_;
    ipc::AtomsIPCChannel* browser_side_channel_;
};

} // namespace process

#endif // THIRD_PARTY_CHROMIUM_PROCESS_NETWORK_PROCESS_HOST_H_
