/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "renderer_process_host.h"
#include "kernel/browser_engine/process/abe_process.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/layout_tree_builder.h"
#include "third_party/blink/renderer/core/paint/blink_skia_painter.h"
#include "third_party/skia/include/adapter/atoms_skia_adapter.h"
#include "third_party/skia/include/core/SkCanvas.h"

namespace process {

RendererProcessHost::RendererProcessHost(uint32_t browser_pid)
    : pid_(0),
      cr3_(0),
      browser_pid_(browser_pid),
      state_(RENDERER_STATE_UNINITIALIZED),
      current_url_("about:blank"),
      last_rendered_title_("New Tab"),
      channel_(nullptr),
      browser_side_channel_(nullptr) {}

RendererProcessHost::~RendererProcessHost() {
    Terminate();
}

RendererProcessHost* RendererProcessHost::Create(uint32_t browser_pid) {
    RendererProcessHost* host = new RendererProcessHost(browser_pid);
    if (!host->Launch()) {
        delete host;
        return nullptr;
    }
    return host;
}

bool RendererProcessHost::Launch() {
    if (state_ == RENDERER_STATE_RUNNING) return true;

    // 1. Request real process instantiation from ATOMS kernel process manager
    uint32_t child_pid = 0;
    ABE_Error err = ABE_Process_Create(ABE_PROC_ROLE_RENDERER, &child_pid);
    if (err != ABE_SUCCESS || child_pid == 0) {
        return false;
    }

    pid_ = child_pid;
    cr3_ = ABE_Process_GetCR3(pid_);
    state_ = RENDERER_STATE_RUNNING;

    // 2. Establish bidirectional IPC channels (Phase 13 transport)
    channel_ = ipc::AtomsIPCChannel::Create("renderer-channel", pid_, browser_pid_);
    browser_side_channel_ = ipc::AtomsIPCChannel::Create("browser-channel", browser_pid_, pid_);

    if (channel_ && browser_side_channel_) {
        channel_->SetPeer(browser_side_channel_);
        browser_side_channel_->SetPeer(channel_);
    }

    return true;
}

bool RendererProcessHost::Navigate(const std::string& url, const std::string& html_source) {
    if (state_ != RENDERER_STATE_RUNNING || !channel_) return false;

    current_url_ = url;

    // Send NAVIGATE IPC message from Browser to Renderer
    channel_->SendString(ipc::MSG_NAVIGATE, html_source);

    // Process inside isolated renderer pipeline
    return ExecuteRenderPipeline(html_source, nullptr, 800, 600);
}

bool RendererProcessHost::SendDOMEvent(uint32_t event_type, int32_t x, int32_t y, uint32_t key_code) {
    if (state_ != RENDERER_STATE_RUNNING || !channel_) return false;

    uint32_t payload[4] = { event_type, (uint32_t)x, (uint32_t)y, key_code };
    return channel_->Send(ipc::MSG_DOM_EVENT, payload, sizeof(payload));
}

bool RendererProcessHost::ExecuteRenderPipeline(const std::string& html_source, void* surface_ptr, int width, int height) {
    if (state_ != RENDERER_STATE_RUNNING) return false;

    // Instantiate Blink Document inside this renderer's context
    blink::Document doc;
    doc.setURL(current_url_);
    doc.parseHTML(html_source);
    last_rendered_title_ = doc.getTitle();

    blink::LayoutObject* layout = blink::LayoutTreeBuilder::buildLayoutTree(&doc);
    if (!layout) return false;

    layout->layout(0, 0, width > 0 ? width : 800);

    if (surface_ptr && width > 0 && height > 0) {
        AtomsSkiaSurface skia_surface((uint32_t*)surface_ptr, width, height);
        SkCanvas* canvas = skia_surface.GetCanvas();
        if (canvas) {
            canvas->clear(0xFF181825);
            blink::BlinkSkiaPainter::paint(layout, canvas);
            skia_surface.Flush();
        }
    }

    delete layout;
    return true;
}

void RendererProcessHost::SimulateCrash() {
    if (state_ != RENDERER_STATE_RUNNING) return;

    state_ = RENDERER_STATE_CRASHED;

    // Report crash to kernel process manager for safe cleanup
    ABE_Process_CrashHandler(pid_);

    // Notify via IPC
    if (channel_) {
        channel_->Send(ipc::MSG_PROCESS_CRASH, nullptr, 0);
    }
}

void RendererProcessHost::Terminate() {
    if (state_ == RENDERER_STATE_TERMINATED || state_ == RENDERER_STATE_UNINITIALIZED) return;

    state_ = RENDERER_STATE_TERMINATED;

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
