/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_PROCESS_RENDERER_PROCESS_HOST_H_
#define THIRD_PARTY_CHROMIUM_PROCESS_RENDERER_PROCESS_HOST_H_

#include "third_party/chromium_ipc/atoms_ipc_channel.h"
#include "userspace/runtime/cpp/include/string"
#include <stdint.h>
#include <stdbool.h>

namespace process {

enum RendererState {
    RENDERER_STATE_UNINITIALIZED = 0,
    RENDERER_STATE_RUNNING = 1,
    RENDERER_STATE_CRASHED = 2,
    RENDERER_STATE_TERMINATED = 3
};

class RendererProcessHost {
public:
    RendererProcessHost(uint32_t browser_pid);
    ~RendererProcessHost();

    static RendererProcessHost* Create(uint32_t browser_pid);

    bool Launch();
    bool Navigate(const std::string& url, const std::string& html_source);
    bool SendDOMEvent(uint32_t event_type, int32_t x, int32_t y, uint32_t key_code);
    void Terminate();
    void SimulateCrash(); // For testing crash recovery

    uint32_t pid() const { return pid_; }
    uint64_t cr3() const { return cr3_; }
    uint32_t browser_pid() const { return browser_pid_; }
    RendererState state() const { return state_; }
    bool is_alive() const { return state_ == RENDERER_STATE_RUNNING; }
    const std::string& current_url() const { return current_url_; }
    const std::string& last_rendered_title() const { return last_rendered_title_; }

    ipc::AtomsIPCChannel* channel() { return channel_; }

    // Renders the page through Blink + V8 + Skia inside this renderer's address space
    bool ExecuteRenderPipeline(const std::string& html_source, void* surface_ptr, int width, int height);

private:
    uint32_t pid_;
    uint64_t cr3_;
    uint32_t browser_pid_;
    RendererState state_;
    std::string current_url_;
    std::string last_rendered_title_;
    ipc::AtomsIPCChannel* channel_;
    ipc::AtomsIPCChannel* browser_side_channel_;
};

} // namespace process

#endif // THIRD_PARTY_CHROMIUM_PROCESS_RENDERER_PROCESS_HOST_H_
