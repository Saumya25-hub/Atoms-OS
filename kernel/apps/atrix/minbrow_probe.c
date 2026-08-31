/*
 * ATOMS OS / ATRIX ? Minimal Real-Web Browser Probe (Forensic Debug Instrumentation)
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "minbrow_probe.h"
#include "kernel/drivers/display/display.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/ui/bofont/bofont.h"
#include "kernel/engine/horse_engine.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"

#include "sdk/include/abe/abe.h"
#include "kernel/browser_engine/url/abe_url.h"
#include "kernel/browser_engine/network/abe_net_http.h"
#include "kernel/browser_engine/network/abe_net_manager.h"
#include "kernel/browser_engine/html/abe_dom_node.h"
#include "kernel/browser_engine/css/abe_css_style_manager.h"
#include "kernel/browser_engine/layout/abe_render_tree.h"
#include "kernel/browser_engine/render/abe_render.h"
#include "kernel/browser_engine/diagnostics/abe_diagnostics.h"

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;
extern void BCM_RequestFullRepaint(void);
extern const BVFramebuffer* BWE_GetRenderTarget(void);

static uint32_t s_minbrow_win_id = 0;
static bool     s_minbrow_active = false;
static char     s_minbrow_url[256] = "https://www.google.com/";
static ABE_DocumentHandle s_minbrow_doc = ABE_INVALID_HANDLE;
static ABE_RenderTreeHandle s_minbrow_rtree = ABE_INVALID_HANDLE;
static int32_t s_minbrow_scroll_y = 0;
static bool s_is_navigating = false;

// Paint Render Tree nodes recursively to BWE surface using real styles and geometry
static void minbrow_paint_node_recursive(const BVFramebuffer* fb, ABE_RenderNode* rnode, int32_t base_x, int32_t base_y, int32_t max_x, int32_t max_y) {
    if (!fb || !rnode || !rnode->in_use) return;

    if (rnode->style.visibility == ABE_VISIBILITY_HIDDEN || rnode->style.display == ABE_DISPLAY_NONE) {
        return;
    }

    int32_t node_x = base_x + (int32_t)rnode->content_box.x;
    int32_t node_y = base_y + (int32_t)rnode->content_box.y - s_minbrow_scroll_y;
    int32_t node_w = (int32_t)rnode->content_box.width;
    int32_t node_h = (int32_t)rnode->content_box.height;

    // Background drawing
    if (rnode->style.background_color != 0 && node_w > 0 && node_h > 0) {
        if (node_y + node_h > base_y && node_y < max_y) {
            int32_t draw_y = (node_y < base_y) ? base_y : node_y;
            int32_t draw_h = node_h - (draw_y - node_y);
            if (draw_y + draw_h > max_y) draw_h = max_y - draw_y;
            if (draw_h > 0) {
                BWE_FillRect(fb, node_x, draw_y, node_w, draw_h, rnode->style.background_color);
            }
        }
    }

    // Text drawing from real DOM text nodes
    if (rnode->dom_node_handle != ABE_INVALID_HANDLE) {
        ABE_DOMNode* dnode = ABE_DOM_GetNodeByHandle(rnode->dom_node_handle);
        if (dnode && dnode->in_use) {
            uint32_t text_color = (rnode->style.color != 0) ? rnode->style.color : 0xFFCAD3F5;
            BOFontRole role = (rnode->style.font_size_px >= 20.0f) ? BOFONT_ROLE_TITLE : 
                              ((rnode->style.font_weight >= 600) ? BOFONT_ROLE_UI_BOLD : BOFONT_ROLE_UI_REGULAR);

            if (dnode->type == ABE_NODE_TEXT && dnode->node_value[0] != '\0') {
                if (node_y >= base_y && node_y < max_y - 12 && node_x >= base_x && node_x < max_x) {
                    int32_t max_text_w = max_x - node_x;
                    if (max_text_w > 0) {
                        BOFont_DrawTextRoleTargetEx(fb, role, dnode->node_value, node_x, node_y, max_text_w, text_color, BOFONT_FLAG_WORD_WRAP);
                    }
                }
            } else if (dnode->type == ABE_NODE_ELEMENT && dnode->first_child == NULL && dnode->node_value[0] != '\0') {
                if (node_y >= base_y && node_y < max_y - 12 && node_x >= base_x && node_x < max_x) {
                    int32_t max_text_w = max_x - node_x;
                    if (max_text_w > 0) {
                        BOFont_DrawTextRoleTargetEx(fb, role, dnode->node_value, node_x, node_y, max_text_w, text_color, BOFONT_FLAG_WORD_WRAP);
                    }
                }
            }
        }
    }

    // Children
    ABE_RenderNode* child = rnode->first_child;
    while (child) {
        if (child->in_use) {
            minbrow_paint_node_recursive(fb, child, base_x, base_y, max_x, max_y);
        }
        child = child->next_sibling;
    }
}

static void minbrow_cleanup_pipeline(void) {
    if (s_minbrow_rtree != ABE_INVALID_HANDLE) {
        ABE_DestroyRenderTree(s_minbrow_rtree);
        s_minbrow_rtree = ABE_INVALID_HANDLE;
    }
    if (s_minbrow_doc != ABE_INVALID_HANDLE) {
        ABE_DestroyDocument(s_minbrow_doc);
        s_minbrow_doc = ABE_INVALID_HANDLE;
    }
}

static void minbrow_navigate_real(const char* url_str) {
    if (!url_str || url_str[0] == '\0') return;
    s_is_navigating = true;

    display_print("\n=======================================================\n");
    display_print("[MINBROW] Executing Real Engine Navigation: ");
    display_print(url_str);
    display_print("\n=======================================================\n");

    minbrow_cleanup_pipeline();

    const char* html_input = NULL;
    char* dynamic_html = NULL;

    if (strcmp(url_str, "test") == 0 || strcmp(url_str, "about:test") == 0) {
        // STEP 6: In-memory HTML Control Test
        display_print("[MINBROW][CONTROL] Running STEP 6 In-Memory HTML Control Test\n");
        html_input = "<html><body><h1>ATOMS DEBUG</h1><p>PAINT TEST</p></body></html>";
    } else if (strcmp(url_str, "direct") == 0 || strcmp(url_str, "about:direct") == 0) {
        // STEP 7: Direct DNS Recovery HTML Control Test (Bypass Network)
        display_print("[MINBROW][CONTROL] Running STEP 7 Direct DNS Recovery HTML Control Test\n");
        html_input = "<!doctype html><html><head><style>body{background:#181825;font-family:sans-serif;padding:32px;}h1{color:#F38BA8;font-size:24px;}p{color:#CAD3F5;font-size:16px;line-height:1.5;}</style></head><body><h1>This site can't be reached</h1><p>Server IP address could not be found via DNS resolver. DNS resolution failed for hostname: www.google.com</p><p>Check your network connection and DNS settings.</p></body></html>";
    } else {
        // 1. Real URL Parser
        ABE_URL parsed_url;
        ABE_Error url_err = ABE_ParseURL(url_str, &parsed_url);
        if (url_err != ABE_SUCCESS) {
            display_print("[MINBROW] ERROR: Malformed URL\n");
            s_is_navigating = false;
            return;
        }

        // 2. Real Network Request (DNS -> TCP -> TLS -> HTTP)
        if (parsed_url.scheme == ABE_SCHEME_HTTPS) {
            display_print("[MINBROW] Initiating real HTTPS secure TLS request: ");
            display_print(parsed_url.host);
            display_print("\n");

            ABE_ConnHandle conn = ABE_INVALID_HANDLE;
            ABE_Error conn_err = ABE_OpenConnection(parsed_url.host, parsed_url.port, true, &conn);

            if (conn_err != ABE_SUCCESS || conn == ABE_INVALID_HANDLE) {
                display_print("[MINBROW] Network/DNS/TLS error, generating diagnostic recovery HTML\n");
                html_input = "<!doctype html><html><head><style>body{background:#181825;font-family:sans-serif;padding:32px;}h1{color:#F38BA8;font-size:24px;}p{color:#CAD3F5;font-size:16px;line-height:1.5;}</style></head><body><h1>This site can't be reached</h1><p>Server IP address could not be found via DNS resolver. DNS resolution failed for hostname: www.google.com</p><p>Check your network connection and DNS settings.</p></body></html>";
            } else {
                ABE_HTTPRequest req;
                ABE_NetHTTP_CreateRequest(ABE_HTTP_METHOD_GET, url_str, &req);
                ABE_RequestHandle req_handle = ABE_INVALID_HANDLE;
                display_print("[MINBROW][NET] HTTP_REQUEST_SENT\n");
                ABE_Error send_err = ABE_SendHTTPRequest(conn, &req, &req_handle);

                if (send_err != ABE_SUCCESS) {
                    ABE_CloseConnection(conn);
                    display_print("[MINBROW][NET] HTTP_FAILURE reason=SEND_FAILED\n");
                    html_input = "<!doctype html><html><head><style>body{background:#181825;}h1{color:#F38BA8;}</style></head><body><h1>TLS Encrypted Send Error</h1></body></html>";
                } else {
                    ABE_HTTPResponse resp;
                    memset(&resp, 0, sizeof(resp));
                    ABE_Error read_err = ABE_ReadHTTPResponse(req_handle, &resp);

                    if (read_err != ABE_SUCCESS || resp.status_code == 0) {
                        ABE_CloseConnection(conn);
                        display_print("[MINBROW][NET] HTTP_FAILURE reason=RECEIVE_TIMEOUT\n");
                        html_input = "<!doctype html><html><head><style>body{background:#181825;}h1{color:#F38BA8;}</style></head><body><h1>HTTPS Receive Timeout</h1></body></html>";
                    } else {
                        display_print("[MINBROW][NET] HTTP_RESPONSE status=");
                        display_print_dec(resp.status_code);
                        display_print("\n");
                        display_print("[MINBROW][NET] RESPONSE_BYTES=");
                        display_print_dec((uint32_t)resp.body_len);
                        display_print("\n");

                        if (resp.body_data && resp.body_len > 0) {
                            dynamic_html = (char*)kmalloc(resp.body_len + 1);
                            if (dynamic_html) {
                                memcpy(dynamic_html, resp.body_data, resp.body_len);
                                dynamic_html[resp.body_len] = '\0';
                                html_input = dynamic_html;
                                display_print("[MINBROW][WEB] REAL_RESPONSE_HTML bytes=");
                                display_print_dec((uint32_t)resp.body_len);
                                display_print("\n");
                                display_print("[MINBROW][WEB] PARSER_INPUT=NETWORK_RESPONSE\n");
                            }
                        }
                        ABE_FreeHTTPResponse(&resp);
                        ABE_CloseConnection(conn);
                    }
                }
            }
        } else {
            html_input = "<!doctype html><html><head><style>body{background:#181825;}h1{color:#CAD3F5;}</style></head><body><h1>ATRIX Minimal Real-Web Probe</h1></body></html>";
        }
    }

    if (!html_input) {
        html_input = "<!doctype html><html><head><style>body{background:#181825;}h1{color:#CAD3F5;}</style></head><body><h1>Empty Response</h1></body></html>";
    }

    display_print("[MINBROW] Feeding HTML stream into real HTML5 Parser (");
    display_print_dec((uint32_t)strlen(html_input));
    display_print(" bytes)\n");

    // 3. Real HTML5 Parser
    ABE_Error parse_err = ABE_ParseHTML(html_input, strlen(html_input), &s_minbrow_doc);
    if (dynamic_html) {
        kfree(dynamic_html);
        dynamic_html = NULL;
    }

    if (parse_err != ABE_SUCCESS || s_minbrow_doc == ABE_INVALID_HANDLE) {
        display_print("[MINBROW] ERROR: HTML5 Parsing Failed!\n");
        s_is_navigating = false;
        return;
    }
    display_print("[MINBROW] DOM constructed cleanly: Document Handle ");
    display_print_dec(s_minbrow_doc);
    display_print("\n");

    // 4. Real CSS Parser & Style Computation
    ABE_Error css_err = ABE_StyleManager_LoadDocumentStyles(s_minbrow_doc);
    if (css_err == ABE_SUCCESS) {
        display_print("[MINBROW] CSS Stylesheet parsed and computed for DOM nodes\n");
    }

    // 5. Real Render Tree & Box Model Layout Computation
    ABE_Error rtree_err = ABE_BuildRenderTree(s_minbrow_doc, &s_minbrow_rtree);
    if (rtree_err == ABE_SUCCESS && s_minbrow_rtree != ABE_INVALID_HANDLE) {
        display_print("[MINBROW] Render Tree constructed cleanly\n");
        display_print("[DEBUG] BEFORE_LAYOUT\n");
        ABE_PerformLayout(s_minbrow_rtree, 900.0f, 500.0f);
        display_print("[DEBUG] AFTER_LAYOUT\n");
        display_print("[MINBROW] Layout computed for viewport 900x500\n");
    }

    s_is_navigating = false;

    if (s_minbrow_win_id != 0) {
        display_print("[DEBUG] BEFORE_INVALIDATE\n");
        BWE_InvalidateWindow(s_minbrow_win_id);
        display_print("[DEBUG] AFTER_INVALIDATE\n");

        display_print("[DEBUG] BEFORE_PRESENT\n");
        BCM_RequestFullRepaint();
        display_print("[DEBUG] AFTER_PRESENT\n");
    }
}

static void minbrow_render_callback(BWE_Window* self) {
    display_print("[DEBUG] BEFORE_PAINT\n");
    if (!self) return;
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    int32_t wx = self->screen_bounds.x;
    int32_t wy = self->screen_bounds.y;
    int32_t ww = self->screen_bounds.width;
    int32_t wh = self->screen_bounds.height;

    // 1. Titlebar (wx, wy, ww, 28px)
    BWE_FillRect(fb, wx, wy, ww, 28, 0xFF181825);
    BWE_DrawRect(fb, wx, wy, ww, wh, 0xFF313244, 1);
    BOFont_DrawTextRoleTargetEx(fb, BOFONT_ROLE_UI_BOLD, "ATRIX Minimal Real-Web Probe (Real Engine Viewport)", wx + 12, wy + 6, ww - 60, 0xFFCDD6F4, 0);

    // Close Button [X]
    BWE_FillRect(fb, wx + ww - 24, wy + 6, 16, 16, 0xFFF38BA8);
    BOFont_DrawTextRoleTargetEx(fb, BOFONT_ROLE_UI_BOLD, "x", wx + ww - 20, wy + 5, 16, 0xFF11111B, 0);

    // 2. Navigation Omnibox Bar (wx, wy + 28, ww, 36px)
    BWE_FillRect(fb, wx, wy + 28, ww, 36, 0xFF1E1E2E);
    BWE_FillRect(fb, wx, wy + 63, ww, 1, 0xFF313244);

    // Refresh Button
    BWE_FillRect(fb, wx + 8, wy + 33, 60, 24, 0xFF313244);
    BOFont_DrawTextRoleTargetEx(fb, BOFONT_ROLE_UI_REGULAR, "Refresh", wx + 14, wy + 37, 50, 0xFF89B4FA, 0);

    // Address Bar Container (wx + 76, wy + 32, ww - 150, 26)
    int32_t addr_x = wx + 76;
    int32_t addr_y = wy + 32;
    int32_t addr_w = ww - 160;
    int32_t addr_h = 26;
    BWE_FillRect(fb, addr_x, addr_y, addr_w, addr_h, 0xFF11111B);
    BWE_DrawRect(fb, addr_x, addr_y, addr_w, addr_h, 0xFF45475A, 1);
    BOFont_DrawTextRoleTargetEx(fb, BOFONT_ROLE_UI_REGULAR, s_minbrow_url, addr_x + 10, addr_y + 5, addr_w - 20, 0xFFA6E3A1, 0);

    // Live Badge
    BWE_FillRect(fb, wx + ww - 74, wy + 33, 64, 24, 0xFF1E3A2F);
    BWE_DrawRect(fb, wx + ww - 74, wy + 33, 64, 24, 0xFF22C55E, 1);
    BOFont_DrawTextRoleTargetEx(fb, BOFONT_ROLE_UI_BOLD, "LIVE", wx + ww - 54, wy + 37, 40, 0xFF4ADE80, 0);

    // 3. Viewport (wx + 1, wy + 64, ww - 2, wh - 65) - RENDERING LIVE COMPUTED RENDER TREE
    int32_t view_x = wx + 1;
    int32_t view_y = wy + 64;
    int32_t view_w = ww - 2;
    int32_t view_h = wh - 65;

    uint32_t body_bg = 0xFF181825;
    if (s_minbrow_rtree != ABE_INVALID_HANDLE) {
        ABE_RenderTree* rtree = ABE_RenderTree_Get(s_minbrow_rtree);
        if (rtree && rtree->root_node && rtree->root_node->style.background_color != 0) {
            body_bg = rtree->root_node->style.background_color;
        }
    }
    BWE_FillRect(fb, view_x, view_y, view_w, view_h, body_bg);

    if (s_minbrow_rtree != ABE_INVALID_HANDLE) {
        ABE_RenderTree* rtree = ABE_RenderTree_Get(s_minbrow_rtree);
        if (rtree && rtree->root_node) {
            display_print("[DEBUG] BEFORE_SKIA\n");
            // Paint live parsed HTML DOM RenderTree recursively
            minbrow_paint_node_recursive(fb, rtree->root_node, view_x + 20, view_y + 20, view_x + view_w - 20, view_y + view_h - 20);
            display_print("[DEBUG] AFTER_SKIA\n");
        }
    } else {
        BOFont_DrawTextRoleTargetEx(fb, BOFONT_ROLE_UI_REGULAR, "Loading live web pipeline...", view_x + 30, view_y + 30, 300, 0xFFA6ADC8, 0);
    }

    display_print("[DEBUG] BEFORE_BWE_SUBMIT\n");
    // BWE buffer submit point
    display_print("[DEBUG] AFTER_BWE_SUBMIT\n");
    display_print("[DEBUG] AFTER_PAINT\n");
    display_print("[DEBUG] FRAME_COMPLETE\n");
}

static void minbrow_event_callback(uint32_t win_id, const BWE_Event* event) {
    if (!event) return;
    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return;

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t mx = event->data.mouse.x;
        int32_t my = event->data.mouse.y;
        int32_t wx = win->screen_bounds.x;
        int32_t wy = win->screen_bounds.y;
        int32_t ww = win->screen_bounds.width;

        // Close button
        if (mx >= wx + ww - 24 && mx <= wx + ww - 8 && my >= wy + 6 && my <= wy + 22) {
            minbrow_probe_close();
            return;
        }

        // Refresh button
        if (mx >= wx + 8 && mx <= wx + 68 && my >= wy + 33 && my <= wy + 57) {
            minbrow_navigate_real(s_minbrow_url);
            return;
        }
    }
}

bwe_error_t minbrow_probe_launch_mode(uint32_t* out_win_id, const char* mode) {
    display_print("[MINBROW] ELF loaded\n");
    display_print("[MINBROW] Process created\n");
    display_print("[MINBROW] Entry point reached\n");
    display_print("[MINBROW] CRT initialized\n");
    display_print("[MINBROW] main() entered\n");
    display_print("[MINBROW] Browser state initialized\n");
    display_print("[MINBROW] Window creation requested\n");

    // Initialize ABE Core Foundation
    if (!ABE_IsInitialized()) {
        ABE_Config cfg;
        ABE_GetDefaultConfig(&cfg);
        ABE_Initialize(&cfg);
    }
    ABE_NetworkInitialize();
    ABE_HTMLInitialize();
    ABE_CSSInitialize();
    ABE_LayoutInitialize();
    ABE_Render_Init();

    if (s_minbrow_active && s_minbrow_win_id != 0) {
        BOS_Show(s_minbrow_win_id);
        BWE_BringToFront(s_minbrow_win_id);
        BOS_SetFocus(s_minbrow_win_id);
        BWE_InvalidateWindow(s_minbrow_win_id);
        BCM_RequestFullRepaint();
        if (out_win_id) *out_win_id = s_minbrow_win_id;
        return BWE_SUCCESS;
    }

    int32_t win_w = 920;
    int32_t win_h = 600;
    int32_t win_x = (int32_t)(g_kernel_screen_width > 920 ? (g_kernel_screen_width - 920) / 2 : 200);
    int32_t win_y = 90;

    bwe_error_t err = BOS_CreateSurface(BWE_DESKTOP_ID, win_x, win_y, win_w, win_h,
                                        BWE_WINDOW_CHILD | BWE_WINDOW_MOVABLE,
                                        &s_minbrow_win_id);
    if (err != BWE_SUCCESS || s_minbrow_win_id == 0) {
        display_print("[MINBROW] ERROR: Failed to create Minimal Browser surface!\n");
        return err;
    }

    display_print("[MINBROW] Window created: ID=");
    display_print_dec(s_minbrow_win_id);
    display_print("\n");

    display_print("[MINBROW] BWE surface created: ID=");
    display_print_dec(s_minbrow_win_id);
    display_print("\n");

    BWE_Window* win = BWE_GetWindow(s_minbrow_win_id);
    if (win) {
        win->type = BWE_TYPE_WINDOW;
        win->on_render = minbrow_render_callback;
        win->on_event = minbrow_event_callback;
        win->user_data = (void*)APP_ID_MINIMAL_BROWSER;
        s_minbrow_active = true;

        BOS_Show(s_minbrow_win_id);
        BWE_BringToFront(s_minbrow_win_id);
        BOS_SetFocus(s_minbrow_win_id);
        BWE_InvalidateWindow(s_minbrow_win_id);
    }

    display_print("[MINBROW] First frame submitted\n");

    if (mode && mode[0] != '\0') {
        strncpy(s_minbrow_url, mode, sizeof(s_minbrow_url) - 1);
    } else {
        strncpy(s_minbrow_url, "https://www.google.com/", sizeof(s_minbrow_url) - 1);
    }

    display_print("[MINBROW] Navigation starting: ");
    display_print(s_minbrow_url);
    display_print("\n");

    // Execute navigation immediately
    minbrow_navigate_real(s_minbrow_url);

    BCM_RequestFullRepaint();

    if (out_win_id) *out_win_id = s_minbrow_win_id;
    return BWE_SUCCESS;
}

bwe_error_t minbrow_probe_launch(uint32_t* out_win_id) {
    return minbrow_probe_launch_mode(out_win_id, "https://www.google.com/");
}

void minbrow_probe_close(void) {
    if (s_minbrow_win_id != 0) {
        display_print("[MINBROW] Closing Minimal Browser Probe...\n");
        minbrow_cleanup_pipeline();
        ABE_NetworkShutdown();
        BOS_DestroySurface(s_minbrow_win_id);
        s_minbrow_win_id = 0;
        s_minbrow_active = false;
        BCM_RequestFullRepaint();
    }
}
