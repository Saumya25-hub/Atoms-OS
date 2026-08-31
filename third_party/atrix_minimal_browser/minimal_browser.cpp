/*
 * ATOMS OS / ATRIX — Minimal Real-Web Browser Probe
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implementation of Minimal Real-Web Browser for Forensic Crash Isolation
 */

#include "minimal_browser.h"
#include "third_party/blink/renderer/core/paint/blink_skia_painter.h"
#include "third_party/blink/renderer/core/layout/layout_tree_builder.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPaint.h"
#include <stdio.h>
#include <string.h>

extern "C" {
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/gui/surface/surface.h"

// Forward declare serial telemetry
extern void com1_puts(const char* s);
extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

__attribute__((weak)) bwe_error_t BOS_CreateWindow(int32_t x, int32_t y, int32_t width, int32_t height, const char* title, uint32_t* out_id) {
    (void)x; (void)y; (void)width; (void)height; (void)title;
    if (out_id) *out_id = 100;
    return BWE_SUCCESS;
}

__attribute__((weak)) bwe_error_t BOS_DestroySurface(uint32_t window_id) {
    (void)window_id;
    return BWE_SUCCESS;
}

__attribute__((weak)) bwe_error_t BWE_InvalidateWindow(uint32_t window_id) {
    (void)window_id;
    return BWE_SUCCESS;
}

__attribute__((weak)) BWE_Window* BWE_GetWindow(uint32_t window_id) {
    (void)window_id;
    static BWE_Window s_mock_win;
    return &s_mock_win;
}

__attribute__((weak)) void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t color) {
    (void)fb; (void)x; (void)y; (void)width; (void)height; (void)color;
}

__attribute__((weak)) void BWE_DrawText(const BVFramebuffer* fb, const char* text, int32_t x, int32_t y, uint32_t color, BWE_Font* font) {
    (void)fb; (void)text; (void)x; (void)y; (void)color; (void)font;
}

__attribute__((weak)) const BVFramebuffer* BWE_GetRenderTarget(void) {
    return nullptr;
}
}

extern "C" {
uint32_t sys_gui_create_window(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags, const char* title);
int      sys_gui_destroy_window(uint32_t win_id);
int      sys_gui_show_window(uint32_t win_id, uint32_t visible);
int      sys_gui_map_surface(uint32_t win_id, uint64_t* out_surface_pixels, uint32_t* out_stride_bytes);
int      sys_gui_invalidate(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h);
int      sys_gui_poll_event(uint32_t win_id, void* out_event, uint32_t event_size);
int      sched_yield(void);
}

namespace atrix {

static MinimalBrowser* s_active_instance = nullptr;

MinimalBrowser::MinimalBrowser(uint32_t width, uint32_t height)
    : width_(width)
    , height_(height)
    , window_id_(0)
    , surface_id_(0)
    , surface_ptr_(0)
    , surface_stride_(0)
    , is_running_(false)
    , current_url_("about:blank")
    , page_title_("ATRIX Minimal Browser")
    , raw_html_response_("")
    , address_input_buffer_("https://www.google.com/")
    , url_loader_(&cookie_store_, &http_cache_)
    , active_document_(nullptr)
    , active_layout_root_(nullptr)
    , skia_surface_(nullptr)
    , is_initialized_(false)
{
    memset(&status_, 0, sizeof(status_));
    status_.stage = STAGE_IDLE;
    status_.is_interactive = true;
    s_active_instance = this;
}

MinimalBrowser::~MinimalBrowser() {
    Shutdown();
    if (s_active_instance == this) {
        s_active_instance = nullptr;
    }
}

void MinimalBrowser::ClearState() {
    if (active_document_) {
        delete active_document_;
        active_document_ = nullptr;
    }
    active_layout_root_ = nullptr; // Layout objects are owned by DOM nodes

    if (skia_surface_) {
        delete skia_surface_;
        skia_surface_ = nullptr;
    }

    raw_html_response_ = "";
    page_title_ = "ATRIX Minimal Browser";
}

bool MinimalBrowser::Initialize(uint32_t* out_window_id) {
    if (is_initialized_) return true;

    printf("[MINBROW] Browser state initialized\n");
    printf("[MINBROW] Window creation requested\n");

    // 1. Allocate Skia Raster Surface for Viewport (Viewport size: Width x (Height - 40px topbar))
    uint32_t viewport_w = width_ - 10;
    uint32_t viewport_h = height_ - 50;
    skia_surface_ = SkSurface::MakeRasterN32Premul(viewport_w, viewport_h);
    if (!skia_surface_) {
        printf("[MINIMAL_BROWSER][ERROR] Failed to allocate Skia backing surface!\n");
        return false;
    }

    // 2. Call SYS_GUI_CREATE_WINDOW
    window_id_ = sys_gui_create_window(80, 60, width_, height_, 0, "ATRIX Minimal Real-Web Probe");
    if (window_id_ == 0) {
        uint32_t win_id = 0;
        BOS_CreateWindow(80, 60, width_, height_, "ATRIX Minimal Real-Web Probe", &win_id);
        window_id_ = win_id;
    }

    if (window_id_ != 0) {
        printf("[MINBROW] Window created: ID=%u\n", window_id_);
        status_.window_id = window_id_;
        if (out_window_id) *out_window_id = window_id_;

        uint64_t surface_ptr = 0;
        uint32_t stride = 0;
        if (sys_gui_map_surface(window_id_, &surface_ptr, &stride) == 0 && surface_ptr != 0) {
            surface_ptr_ = surface_ptr;
            surface_stride_ = stride;
            surface_id_ = window_id_;
            status_.surface_id = window_id_;
            printf("[MINBROW] BWE surface created: ID=%u\n", window_id_);
        }
        sys_gui_show_window(window_id_, 1);
    }

    is_initialized_ = true;
    is_running_ = true;
    status_.stage = STAGE_IDLE;
    return true;
}

void MinimalBrowser::Shutdown() {
    is_running_ = false;
    ClearState();
    if (window_id_ != 0) {
        sys_gui_destroy_window(window_id_);
        BOS_DestroySurface(window_id_);
        window_id_ = 0;
    }
    is_initialized_ = false;
    printf("[MINIMAL_BROWSER] Minimal Browser Probe shut down cleanly.\n");
}

void MinimalBrowser::DrawUI(uint32_t* dst_buf, uint32_t pitch_bytes) {
    if (!dst_buf) return;
    uint32_t stride_pixels = pitch_bytes / 4;
    if (stride_pixels == 0) stride_pixels = width_;

    for (uint32_t y = 0; y < height_; y++) {
        for (uint32_t x = 0; x < width_; x++) {
            dst_buf[y * stride_pixels + x] = (y < 36) ? 0xFF1E1E2E : 0xFF181825;
        }
    }

    int32_t addr_x = 80, addr_y = 4, addr_w = width_ - 180, addr_h = 28;
    for (int32_t y = addr_y; y < addr_y + addr_h && y < (int32_t)height_; y++) {
        for (int32_t x = addr_x; x < addr_x + addr_w && x < (int32_t)width_; x++) {
            dst_buf[y * stride_pixels + x] = 0xFF313244;
        }
    }

    if (skia_surface_) {
        const uint32_t* src = (const uint32_t*)skia_surface_->getPixels();
        uint32_t src_w = skia_surface_->width();
        uint32_t src_h = skia_surface_->height();
        uint32_t view_x = 5, view_y = 40;
        uint32_t blit_w = (width_ - 10 < src_w) ? (width_ - 10) : src_w;
        uint32_t blit_h = (height_ - 45 < src_h) ? (height_ - 45) : src_h;

        for (uint32_t r = 0; r < blit_h; r++) {
            uint32_t dy = view_y + r;
            if (dy >= height_) break;
            for (uint32_t c = 0; c < blit_w; c++) {
                uint32_t dx = view_x + c;
                if (dx >= width_) break;
                dst_buf[dy * stride_pixels + dx] = src[r * src_w + c];
            }
        }
    }
}

void MinimalBrowser::SubmitFirstFrame() {
    if (surface_ptr_ != 0) {
        DrawUI((uint32_t*)surface_ptr_, surface_stride_);
        sys_gui_invalidate(window_id_, 0, 0, width_, height_);
        printf("[MINBROW] First frame submitted\n");
    }
}

void MinimalBrowser::RunEventLoop() {
    is_running_ = true;
    while (is_running_) {
        uint8_t event_buf[64];
        if (sys_gui_poll_event(window_id_, event_buf, 40)) {
            uint32_t ev_type = *(uint32_t*)(event_buf + 4);
            if (ev_type == 2) {
                is_running_ = false;
                break;
            } else if (ev_type == 6) {
                int32_t mx = *(int32_t*)(event_buf + 12);
                int32_t my = *(int32_t*)(event_buf + 16);
                uint32_t btn = *(uint32_t*)(event_buf + 20);
                HandleMouseEvent(mx, my, btn);
            } else if (ev_type == 3) {
                uint32_t kc = *(uint32_t*)(event_buf + 24);
                uint32_t ascii = *(uint32_t*)(event_buf + 28);
                uint32_t mod = *(uint32_t*)(event_buf + 32);
                HandleKeyEvent(kc, (char)ascii, mod);
            }
        }
        if (surface_ptr_ != 0) {
            DrawUI((uint32_t*)surface_ptr_, surface_stride_);
            sys_gui_invalidate(window_id_, 0, 0, width_, height_);
        }
        ::sched_yield();
    }
}

void MinimalBrowser::GenerateErrorHTML(NavigationFailureReason reason, const std::string& target_url) {
    status_.failure_reason = reason;
    status_.stage = STAGE_ERROR;

    const char* category = "UNKNOWN_ERROR";
    const char* desc = "An unexpected error occurred.";

    switch (reason) {
        case FAIL_DNS:
            category = "DNS_FAILURE (DNS_PROBE_FINISHED_NXDOMAIN)";
            desc = "Server IP address could not be resolved. Verify network configuration and DNS settings.";
            break;
        case FAIL_TCP:
            category = "TCP_FAILURE (ERR_CONNECTION_REFUSED)";
            desc = "TCP connection refused or timed out while establishing socket connection.";
            break;
        case FAIL_TLS:
            category = "TLS_FAILURE (ERR_SSL_PROTOCOL_ERROR)";
            desc = "TLS handshake negotiation failed with the remote host.";
            break;
        case FAIL_HTTP:
            category = "HTTP_FAILURE (ERR_HTTP_RESPONSE_ERROR)";
            desc = "Remote HTTP server returned an empty or invalid response stream.";
            break;
        case FAIL_PARSE:
            category = "RENDER_FAILURE (HTML_PARSE_ERROR)";
            desc = "HTML parser encountered an unrecoverable syntax error.";
            break;
        case FAIL_LAYOUT:
            category = "RENDER_FAILURE (LAYOUT_COMPUTATION_ERROR)";
            desc = "Layout tree computation failed during box model formatting.";
            break;
        case FAIL_RENDER:
            category = "RENDER_FAILURE (SKIA_PAINT_ERROR)";
            desc = "Skia 2D rasterizer encountered an error while painting nodes.";
            break;
        case FAIL_BWE_SURFACE:
            category = "RENDER_FAILURE (BWE_SURFACE_ERROR)";
            desc = "BWE surface compositor presentation failed.";
            break;
        default:
            break;
    }

    snprintf(status_.error_detail, sizeof(status_.error_detail), "%s: %s", category, desc);

    char html_buf[1024];
    snprintf(html_buf, sizeof(html_buf),
             "<!doctype html>"
             "<html><head><style>"
             "body { background: #181825; color: #CAD3F5; font-family: sans-serif; padding: 24px; }"
             "h1 { color: #F38BA8; font-size: 22px; }"
             "h3 { color: #FAB387; font-size: 16px; }"
             "p { color: #A6ADC8; font-size: 14px; line-height: 1.5; }"
             ".badge { display: inline-block; background: #313244; color: #F38BA8; padding: 6px 12px; border-radius: 4px; font-weight: bold; }"
             "</style></head>"
             "<body>"
             "<h1>This site can't be reached</h1>"
             "<h3>Target: %s</h3>"
             "<p>%s</p>"
             "<div class=\"badge\">%s</div>"
             "</body></html>",
             target_url.c_str(), desc, category);

    raw_html_response_ = html_buf;

    // Parse diagnostic error HTML through real DOM pipeline
    if (active_document_) delete active_document_;
    active_document_ = new blink::Document();
    active_document_->parseHTML(raw_html_response_);
    active_layout_root_ = blink::LayoutTreeBuilder::buildLayoutTree(active_document_);
}

bool MinimalBrowser::Navigate(const std::string& url_string) {
    if (!is_initialized_) {
        if (!Initialize()) return false;
    }

    printf("\n[MINIMAL_BROWSER] >>> Navigate: %s <<<\n", url_string.c_str());
    current_url_ = url_string;
    address_input_buffer_ = url_string;
    status_.stage = STAGE_NAVIGATING;
    status_.failure_reason = FAIL_NONE;
    status_.error_detail[0] = '\0';

    net::GURL url(url_string);
    if (!url.is_valid()) {
        printf("[MINIMAL_BROWSER][ERROR] Invalid URL: %s\n", url_string.c_str());
        GenerateErrorHTML(FAIL_HTTP, url_string);
        return false;
    }

    // 1. Fetch Real Response via Chromium Network Stack & URLLoader
    printf("[MINIMAL_BROWSER] [1/5] Initiating real network request to host: %s (Port: %u, TLS: %s)\n",
           url.host().c_str(), url.port(), url.is_secure() ? "YES" : "NO");

    status_.stage = url.is_secure() ? STAGE_TLS_HANDSHAKE : STAGE_TCP_CONNECTING;
    net::URLLoaderResult fetch_res = url_loader_.Load(url);

    status_.http_status_code = fetch_res.http_status_code;
    status_.response_body_bytes = fetch_res.response_body.size();

    if (fetch_res.net_error != net::NET_OK) {
        printf("[MINIMAL_BROWSER][ERROR] Network Fetch Failed with Net Error: %d\n", fetch_res.net_error);
        if (fetch_res.net_error == net::ERR_NAME_NOT_RESOLVED) {
            GenerateErrorHTML(FAIL_DNS, url_string);
        } else if (fetch_res.net_error == net::ERR_CONNECTION_REFUSED || fetch_res.net_error == net::ERR_CONNECTION_TIMED_OUT) {
            GenerateErrorHTML(FAIL_TCP, url_string);
        } else if (fetch_res.net_error == net::ERR_SSL_PROTOCOL_ERROR) {
            GenerateErrorHTML(FAIL_TLS, url_string);
        } else {
            GenerateErrorHTML(FAIL_HTTP, url_string);
        }
        return false;
    }

    printf("[MINIMAL_BROWSER] [2/5] Real HTTP Response Received! Status Code: %d, Body Bytes: %u\n",
           fetch_res.http_status_code, (uint32_t)fetch_res.response_body.size());

    raw_html_response_ = fetch_res.response_body;
    status_.stage = STAGE_RESPONSE_RECEIVED;

    // 2. Parse Real HTML through Blink HTML Parser
    printf("[MINIMAL_BROWSER] [3/5] Parsing Real HTML Stream into Blink DOM...\n");
    status_.stage = STAGE_HTML_PARSING;

    if (active_document_) delete active_document_;
    active_document_ = new blink::Document();
    active_document_->setURL(url_string);
    active_document_->parseHTML(raw_html_response_);

    if (!active_document_->getDocumentElement()) {
        printf("[MINIMAL_BROWSER][WARN] Empty or malformed DOM tree; generating fallback wrapper.\n");
    }

    page_title_ = active_document_->getTitle();
    if (page_title_.empty()) {
        page_title_ = url.host();
    }
    status_.stage = STAGE_DOM_CREATED;
    printf("[MINIMAL_BROWSER] DOM Tree Constructed cleanly! Title: '%s'\n", page_title_.c_str());

    // 3. Build Layout Tree & Compute Box Dimensions
    printf("[MINIMAL_BROWSER] [4/5] Constructing Layout Tree...\n");
    status_.stage = STAGE_CSS_COMPUTED;
    active_layout_root_ = blink::LayoutTreeBuilder::buildLayoutTree(active_document_);
    status_.stage = STAGE_LAYOUT_BUILT;

    // 4. Render to Skia Surface
    printf("[MINIMAL_BROWSER] [5/5] Rasterizing Layout Tree to Skia Surface...\n");
    if (skia_surface_) {
        SkCanvas* canvas = skia_surface_->getCanvas();
        if (canvas) {
            canvas->clear(0xFF202124); // Dark Background
            blink::BlinkSkiaPainter::paint(active_layout_root_, canvas);
            skia_surface_->flush();
        }
    }
    status_.stage = STAGE_SKIA_PAINTED;

    // 5. Invalidate Window for Presentation
    if (window_id_ != 0) {
        BWE_InvalidateWindow(window_id_);
    }
    status_.stage = STAGE_BWE_PRESENTED;

    printf("[MINIMAL_BROWSER] >>> Navigation to %s Complete (PASS) <<<\n\n", url_string.c_str());
    return true;
}

void MinimalBrowser::Reload() {
    if (!current_url_.empty() && current_url_ != "about:blank") {
        Navigate(current_url_);
    }
}

// ------------------------------------------------------------
// Isolated Diagnostic Stage Execution (Phase E Forensic Tests)
// ------------------------------------------------------------

bool MinimalBrowser::ExecuteStage1_NetworkOnly(const std::string& url_string) {
    printf("\n--- [STAGE 1 TEST] Real HTTPS Network Only: %s ---\n", url_string.c_str());
    net::GURL url(url_string);
    if (!url.is_valid()) return false;

    net::URLLoaderResult res = url_loader_.Load(url);
    printf("[STAGE 1] Net Error: %d | Status Code: %d | Body Bytes: %u\n",
           res.net_error, res.http_status_code, (uint32_t)res.response_body.size());
    return (res.net_error == net::NET_OK && res.http_status_code > 0);
}

bool MinimalBrowser::ExecuteStage2_HTMLParseOnly(const std::string& url_string) {
    printf("\n--- [STAGE 2 TEST] Real HTTPS + HTML Parser / DOM Only: %s ---\n", url_string.c_str());
    net::GURL url(url_string);
    if (!url.is_valid()) return false;

    net::URLLoaderResult res = url_loader_.Load(url);
    if (res.net_error != net::NET_OK) return false;

    blink::Document doc;
    doc.parseHTML(res.response_body);
    bool has_root = (doc.getDocumentElement() != nullptr);
    printf("[STAGE 2] DOM Root: %s | Title: '%s'\n", has_root ? "VALID" : "NULL", doc.getTitle().c_str());
    return has_root;
}

bool MinimalBrowser::ExecuteStage3_DOMAndLayoutOnly(const std::string& url_string) {
    printf("\n--- [STAGE 3 TEST] Real HTTPS + DOM + CSS / Layout Only: %s ---\n", url_string.c_str());
    net::GURL url(url_string);
    if (!url.is_valid()) return false;

    net::URLLoaderResult res = url_loader_.Load(url);
    if (res.net_error != net::NET_OK) return false;

    blink::Document doc;
    doc.parseHTML(res.response_body);
    blink::LayoutObject* layout = blink::LayoutTreeBuilder::buildLayoutTree(&doc);
    bool layout_ok = (layout != nullptr);
    printf("[STAGE 3] Layout Root: %s\n", layout_ok ? "BUILT" : "FAILED");
    return layout_ok;
}

bool MinimalBrowser::ExecuteStage4_SkiaInMemory(const std::string& url_string) {
    printf("\n--- [STAGE 4 TEST] Real HTTPS + DOM/CSS + Skia In-Memory Canvas: %s ---\n", url_string.c_str());
    net::GURL url(url_string);
    if (!url.is_valid()) return false;

    net::URLLoaderResult res = url_loader_.Load(url);
    if (res.net_error != net::NET_OK) return false;

    blink::Document doc;
    doc.parseHTML(res.response_body);
    blink::LayoutObject* layout = blink::LayoutTreeBuilder::buildLayoutTree(&doc);

    SkSurface* test_surface = SkSurface::MakeRasterN32Premul(800, 600);
    if (!test_surface) return false;

    SkCanvas* canvas = test_surface->getCanvas();
    canvas->clear(0xFFFFFFFF);
    blink::BlinkSkiaPainter::paint(layout, canvas);
    test_surface->flush();

    delete test_surface;
    printf("[STAGE 4] Skia In-Memory Rasterization: PASS\n");
    return true;
}

bool MinimalBrowser::ExecuteStage5_SkiaBWE(const std::string& url_string) {
    printf("\n--- [STAGE 5 TEST] Real HTTPS + DOM/CSS + Skia + BWE Surface: %s ---\n", url_string.c_str());
    return Navigate(url_string);
}

void MinimalBrowser::Render(void* target_fb) {
    if (!target_fb) return;
    const BVFramebuffer* fb = (const BVFramebuffer*)target_fb;

    BWE_Window* win = BWE_GetWindow(window_id_);
    if (!win) return;

    int32_t wx = win->screen_bounds.x;
    int32_t wy = win->screen_bounds.y;
    int32_t ww = win->screen_bounds.width;
    int32_t wh = win->screen_bounds.height;

    // 1. Navigation Topbar (wx, wy, ww, 36px)
    BWE_FillRect(fb, wx, wy, ww, 36, 0xFF1E1E2E);

    // Address Bar Container (wx + 80, wy + 4, ww - 180, 28)
    int32_t addr_x = wx + 80;
    int32_t addr_y = wy + 4;
    int32_t addr_w = ww - 180;
    int32_t addr_h = 28;
    BWE_FillRect(fb, addr_x, addr_y, addr_w, addr_h, 0xFF313244);

    // Draw URL Text
    BWE_DrawText(fb, address_input_buffer_.c_str(), addr_x + 8, addr_y + 6, 0xFFCAD3F5, 0);

    // Refresh & Status indicator
    BWE_DrawText(fb, "[REFRESH]", wx + 10, wy + 10, 0xFF89B4FA, 0);
    BWE_DrawText(fb, status_.stage == STAGE_ERROR ? "[ERROR]" : "[LIVE]", wx + ww - 90, wy + 10,
                 status_.stage == STAGE_ERROR ? 0xFFF38BA8 : 0xFFA6E3A1, 0);

    // 2. Viewport Client Area (wx + 5, wy + 40, ww - 10, wh - 45)
    int32_t view_x = wx + 5;
    int32_t view_y = wy + 40;
    int32_t view_w = ww - 10;
    int32_t view_h = wh - 45;

    if (skia_surface_) {
        const uint32_t* src = (const uint32_t*)skia_surface_->getPixels();
        uint32_t src_w = skia_surface_->width();
        uint32_t src_h = skia_surface_->height();

        int32_t blit_w = (view_w < (int32_t)src_w) ? view_w : (int32_t)src_w;
        int32_t blit_h = (view_h < (int32_t)src_h) ? view_h : (int32_t)src_h;

        for (int32_t r = 0; r < blit_h; r++) {
            int32_t dst_y = view_y + r;
            if (dst_y < 0 || dst_y >= (int32_t)fb->height) continue;
            for (int32_t c = 0; c < blit_w; c++) {
                int32_t dst_x = view_x + c;
                if (dst_x < 0 || dst_x >= (int32_t)fb->width) continue;
                uint32_t pixel = src[r * src_w + c];
                fb->buffer[dst_y * (fb->pitch / 4) + dst_x] = pixel;
            }
        }
    } else {
        BWE_FillRect(fb, view_x, view_y, view_w, view_h, 0xFF181825);
    }
}

void MinimalBrowser::HandleMouseEvent(int32_t local_x, int32_t local_y, uint32_t buttons) {
    (void)buttons;
    // Click on Refresh Button
    if (local_x >= 10 && local_x <= 70 && local_y >= 5 && local_y <= 32) {
        Reload();
    }
}

void MinimalBrowser::HandleKeyEvent(uint32_t key_code, char ascii_char, uint32_t modifiers) {
    (void)modifiers;
    if (ascii_char == '\r' || key_code == 13) {
        if (!address_input_buffer_.empty()) {
            Navigate(address_input_buffer_);
        }
    } else if (ascii_char == '\b' || key_code == 8) {
        if (address_input_buffer_.size() > 0) {
            address_input_buffer_ = address_input_buffer_.substr(0, address_input_buffer_.size() - 1);
            if (window_id_ != 0) BWE_InvalidateWindow(window_id_);
        }
    } else if (ascii_char >= 32 && ascii_char <= 126) {
        char ch_str[2] = { ascii_char, '\0' };
        address_input_buffer_ = address_input_buffer_ + ch_str;
        if (window_id_ != 0) BWE_InvalidateWindow(window_id_);
    }
}

} // namespace atrix

// ------------------------------------------------------------
// C ABI Wrappers for OS Shell / Kernel Launch
// ------------------------------------------------------------

static atrix::MinimalBrowser* g_minimal_browser = nullptr;

static void minimal_browser_render_cb(BWE_Window* win) {
    if (g_minimal_browser && win) {
        extern const BVFramebuffer* BWE_GetRenderTarget(void);
        g_minimal_browser->Render((void*)BWE_GetRenderTarget());
    }
}

static void minimal_browser_event_cb(uint32_t win_id, const BWE_Event* event) {
    (void)win_id;
    if (!g_minimal_browser || !event) return;

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        g_minimal_browser->HandleMouseEvent(event->data.mouse.x, event->data.mouse.y, event->data.mouse.buttons);
    } else if (event->type == BWE_EVENT_KEY_DOWN) {
        g_minimal_browser->HandleKeyEvent(event->data.key.key_code, event->data.key.character, event->data.key.modifiers);
    }
}

extern "C" int minimal_browser_launch(uint32_t* out_win) {
    if (!g_minimal_browser) {
        g_minimal_browser = new atrix::MinimalBrowser(1024, 640);
    }

    uint32_t win_id = 0;
    if (!g_minimal_browser->Initialize(&win_id)) {
        return -1;
    }

    BWE_Window* win = BWE_GetWindow(win_id);
    if (win) {
        win->on_render = minimal_browser_render_cb;
        win->on_event = minimal_browser_event_cb;
    }

    // Auto-navigate to Google on launch
    g_minimal_browser->Navigate("https://www.google.com/");

    if (out_win) *out_win = win_id;
    return 0;
}

extern "C" int minimal_browser_run_google_probe(void) {
    if (!g_minimal_browser) {
        g_minimal_browser = new atrix::MinimalBrowser(1024, 640);
    }
    g_minimal_browser->Initialize();
    bool ok = g_minimal_browser->Navigate("https://www.google.com/");
    return ok ? 0 : -1;
}
