/*
 * ATOMS OS / ATRIX — Minimal Real-Web Browser Probe
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Dedicated Minimal Browser for Crash Isolation and Real-Web Verification
 */

#ifndef THIRD_PARTY_ATRIX_MINIMAL_BROWSER_MINIMAL_BROWSER_H_
#define THIRD_PARTY_ATRIX_MINIMAL_BROWSER_MINIMAL_BROWSER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
#include "third_party/chromium_net/base/gurl.h"
#include "third_party/chromium_net/url_request/url_loader.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "userspace/runtime/cpp/include/string"

namespace atrix {

enum MinimalBrowserStage {
    STAGE_IDLE = 0,
    STAGE_NAVIGATING,
    STAGE_DNS_RESOLVING,
    STAGE_TCP_CONNECTING,
    STAGE_TLS_HANDSHAKE,
    STAGE_HTTP_FETCHING,
    STAGE_RESPONSE_RECEIVED,
    STAGE_HTML_PARSING,
    STAGE_DOM_CREATED,
    STAGE_CSS_COMPUTED,
    STAGE_LAYOUT_BUILT,
    STAGE_SKIA_PAINTED,
    STAGE_BWE_PRESENTED,
    STAGE_ERROR
};

enum NavigationFailureReason {
    FAIL_NONE = 0,
    FAIL_DNS,
    FAIL_TCP,
    FAIL_TLS,
    FAIL_HTTP,
    FAIL_PARSE,
    FAIL_LAYOUT,
    FAIL_RENDER,
    FAIL_BWE_SURFACE
};

struct MinimalBrowserStatus {
    MinimalBrowserStage stage;
    NavigationFailureReason failure_reason;
    int http_status_code;
    size_t response_body_bytes;
    uint32_t dom_node_count;
    uint32_t layout_node_count;
    uint32_t window_id;
    uint32_t surface_id;
    bool is_interactive;
    char error_detail[256];
};

class MinimalBrowser {
public:
    MinimalBrowser(uint32_t width = 1024, uint32_t height = 640);
    ~MinimalBrowser();

    // Initialization & Lifecycle
    bool Initialize(uint32_t* out_window_id = nullptr);
    void Shutdown();

    // Navigation Pipeline
    bool Navigate(const std::string& url_string);
    void Reload();

    // Isolated Stage Pipeline Execution (For Diagnostics)
    bool ExecuteStage1_NetworkOnly(const std::string& url_string);
    bool ExecuteStage2_HTMLParseOnly(const std::string& url_string);
    bool ExecuteStage3_DOMAndLayoutOnly(const std::string& url_string);
    bool ExecuteStage4_SkiaInMemory(const std::string& url_string);
    bool ExecuteStage5_SkiaBWE(const std::string& url_string);

    // Presentation & Event Dispatch
    void Render(void* target_fb);
    void SubmitFirstFrame();
    void RunEventLoop();
    void HandleMouseEvent(int32_t local_x, int32_t local_y, uint32_t buttons);
    void HandleKeyEvent(uint32_t key_code, char ascii_char, uint32_t modifiers);

    // Query State
    const std::string& GetCurrentURL() const { return current_url_; }
    const std::string& GetTitle() const { return page_title_; }
    const MinimalBrowserStatus& GetStatus() const { return status_; }
    const std::string& GetRawHTML() const { return raw_html_response_; }
    bool IsRunning() const { return is_running_; }

private:
    void ClearState();
    void GenerateErrorHTML(NavigationFailureReason reason, const std::string& target_url);
    void UpdateStatusBanner();
    void DrawUI(uint32_t* dst_buf, uint32_t pitch_bytes);

    uint32_t width_;
    uint32_t height_;
    uint32_t window_id_;
    uint32_t surface_id_;
    uint64_t surface_ptr_;
    uint32_t surface_stride_;
    bool is_running_;
    std::string current_url_;
    std::string page_title_;
    std::string raw_html_response_;
    std::string address_input_buffer_;

    net::CookieStore cookie_store_;
    net::HttpCache http_cache_;
    net::URLLoader url_loader_;

    blink::Document* active_document_;
    blink::LayoutObject* active_layout_root_;
    SkSurface* skia_surface_;

    MinimalBrowserStatus status_;
    bool is_initialized_;
};

} // namespace atrix

extern "C" {
#endif // __cplusplus

// C ABI Compatibility Wrappers for ATOMS OS Kernel/Shell Integration
int minimal_browser_launch(uint32_t* out_win);
int minimal_browser_run_google_probe(void);

#ifdef __cplusplus
}
#endif

#endif // THIRD_PARTY_ATRIX_MINIMAL_BROWSER_MINIMAL_BROWSER_H_
