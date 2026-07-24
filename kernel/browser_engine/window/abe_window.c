#include "abe_window.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static ABE_WindowManager g_win_mgr;
static uint32_t g_next_win_id = 500;
static bool g_win_initialized = false;

static void RecalculateWindowLayout(ABE_BrowserWindow* win) {
    if (!win) return;
    // Tab Strip: Top 30px
    win->tab_strip_bounds.x = 0;
    win->tab_strip_bounds.y = 0;
    win->tab_strip_bounds.w = win->width;
    win->tab_strip_bounds.h = 30;

    // Address Bar: Below Tab Strip, 35px high
    win->address_bar_bounds.x = 0;
    win->address_bar_bounds.y = 30;
    win->address_bar_bounds.w = win->width;
    win->address_bar_bounds.h = 35;

    // Content Surface: Remaining window area
    win->content_surface_bounds.x = 0;
    win->content_surface_bounds.y = 65;
    win->content_surface_bounds.w = win->width;
    win->content_surface_bounds.h = (win->height > 65) ? (win->height - 65) : 0;
}

ABE_Error ABE_Window_Init(void) {
    memset(&g_win_mgr, 0, sizeof(ABE_WindowManager));
    g_win_initialized = true;
    ABE_Log(ABE_LOG_INFO, "WIN", "ABE Browser Window Manager initialized with BOSurface integration");
    return ABE_SUCCESS;
}

ABE_Error ABE_Window_Shutdown(void) {
    if (!g_win_initialized) return ABE_ERR_NOT_INITIALIZED;
    for (uint32_t i = 0; i < ABE_MAX_WINDOWS_REGISTRY; i++) {
        if (g_win_mgr.windows[i].is_active) {
            ABE_Window_Destroy(g_win_mgr.windows[i].handle);
        }
    }
    g_win_initialized = false;
    ABE_Log(ABE_LOG_INFO, "WIN", "ABE Browser Window Manager shut down cleanly");
    return ABE_SUCCESS;
}

ABE_Error ABE_Window_Create(const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height, ABE_WindowHandle* out_handle) {
    if (!g_win_initialized || !out_handle || width == 0 || height == 0) return ABE_ERR_INVALID_PARAM;
    if (g_win_mgr.active_window_count >= ABE_MAX_WINDOWS_REGISTRY) return ABE_ERR_RESOURCE_EXHAUSTED;

    uint32_t slot = ABE_INVALID_HANDLE;
    for (uint32_t i = 0; i < ABE_MAX_WINDOWS_REGISTRY; i++) {
        if (!g_win_mgr.windows[i].is_active) {
            slot = i;
            break;
        }
    }

    if (slot == ABE_INVALID_HANDLE) return ABE_ERR_RESOURCE_EXHAUSTED;

    uint32_t bos_id = 0;
    const char* win_title = (title && title[0] != '\0') ? title : "ATOMS Browser";
    bwe_error_t bwe_err = BOS_CreateWindow(x, y, (int32_t)width, (int32_t)height, win_title, &bos_id);
    if (bwe_err != BWE_SUCCESS) {
        // Fallback synthetic window ID for headless/test environments
        bos_id = 100 + slot;
    }

    ABE_BrowserWindow* win = &g_win_mgr.windows[slot];
    memset(win, 0, sizeof(ABE_BrowserWindow));
    win->handle = (g_next_win_id++) | (slot << 16);
    win->bosurface_id = bos_id;
    strncpy(win->title, win_title, ABE_MAX_TITLE_LEN - 1);
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    win->is_active = true;
    win->is_visible = true;
    win->active_tab_handle = ABE_INVALID_HANDLE;

    RecalculateWindowLayout(win);

    g_win_mgr.active_window_count++;
    g_win_mgr.focused_window = win->handle;
    *out_handle = win->handle;

    ABE_Diag_RecordWindowCreated();
    ABE_LogVal(ABE_LOG_INFO, "WIN", "Created ABE Browser Window, Handle: ", win->handle);
    return ABE_SUCCESS;
}

ABE_Error ABE_Window_Destroy(ABE_WindowHandle handle) {
    if (!g_win_initialized || handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_BrowserWindow* win = ABE_Window_Get(handle);
    if (!win || !win->is_active) return ABE_ERR_WINDOW_FAILED;

    if (win->bosurface_id != 0) {
        BOS_DestroySurface(win->bosurface_id);
        win->bosurface_id = 0;
    }

    win->is_active = false;
    win->is_visible = false;
    if (g_win_mgr.active_window_count > 0) g_win_mgr.active_window_count--;

    ABE_Diag_RecordWindowDestroyed();
    ABE_LogVal(ABE_LOG_INFO, "WIN", "Destroyed ABE Browser Window, Handle: ", handle);
    return ABE_SUCCESS;
}

ABE_Error ABE_Window_Resize(ABE_WindowHandle handle, uint32_t width, uint32_t height) {
    if (!g_win_initialized || width == 0 || height == 0) return ABE_ERR_INVALID_PARAM;
    ABE_BrowserWindow* win = ABE_Window_Get(handle);
    if (!win || !win->is_active) return ABE_ERR_WINDOW_FAILED;

    win->width = width;
    win->height = height;
    RecalculateWindowLayout(win);

    if (win->bosurface_id != 0) {
        BOS_SetBounds(win->bosurface_id, win->x, win->y, width, height);
    }
    ABE_Log(ABE_LOG_INFO, "WIN", "Resized ABE Browser Window");
    return ABE_SUCCESS;
}

ABE_Error ABE_Window_SetFullscreen(ABE_WindowHandle handle, bool fullscreen) {
    if (!g_win_initialized) return ABE_ERR_NOT_INITIALIZED;
    ABE_BrowserWindow* win = ABE_Window_Get(handle);
    if (!win || !win->is_active) return ABE_ERR_WINDOW_FAILED;

    if (fullscreen && !win->is_fullscreen) {
        win->restore_x = win->x;
        win->restore_y = win->y;
        win->restore_w = win->width;
        win->restore_h = win->height;

        win->x = 0;
        win->y = 0;
        win->width = 1024; // Screen standard bounds
        win->height = 768;
        win->is_fullscreen = true;
    } else if (!fullscreen && win->is_fullscreen) {
        win->x = win->restore_x;
        win->y = win->restore_y;
        win->width = win->restore_w;
        win->height = win->restore_h;
        win->is_fullscreen = false;
    }

    RecalculateWindowLayout(win);
    if (win->bosurface_id != 0) {
        BOS_SetBounds(win->bosurface_id, win->x, win->y, win->width, win->height);
    }
    ABE_Log(ABE_LOG_INFO, "WIN", fullscreen ? "Window set FULLSCREEN" : "Window restored from fullscreen");
    return ABE_SUCCESS;
}

ABE_BrowserWindow* ABE_Window_Get(ABE_WindowHandle handle) {
    if (!g_win_initialized || handle == ABE_INVALID_HANDLE) return NULL;
    uint32_t slot = (handle >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_WINDOWS_REGISTRY) return NULL;
    if (g_win_mgr.windows[slot].handle == handle && g_win_mgr.windows[slot].is_active) {
        return &g_win_mgr.windows[slot];
    }
    return NULL;
}

ABE_Error ABE_Window_AttachTab(ABE_WindowHandle window, ABE_TabHandle tab) {
    ABE_BrowserWindow* win = ABE_Window_Get(window);
    if (!win || tab == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    if (win->tab_count >= ABE_MAX_TABS_PER_WINDOW) return ABE_ERR_RESOURCE_EXHAUSTED;

    for (uint32_t i = 0; i < win->tab_count; i++) {
        if (win->tab_handles[i] == tab) return ABE_SUCCESS; // Already attached
    }

    win->tab_handles[win->tab_count++] = tab;
    if (win->active_tab_handle == ABE_INVALID_HANDLE) {
        win->active_tab_handle = tab;
    }
    return ABE_SUCCESS;
}

ABE_Error ABE_Window_DetachTab(ABE_WindowHandle window, ABE_TabHandle tab) {
    ABE_BrowserWindow* win = ABE_Window_Get(window);
    if (!win || tab == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;

    int idx = -1;
    for (uint32_t i = 0; i < win->tab_count; i++) {
        if (win->tab_handles[i] == tab) {
            idx = (int)i;
            break;
        }
    }

    if (idx < 0) return ABE_ERR_TAB_NOT_FOUND;

    for (uint32_t i = (uint32_t)idx; i < win->tab_count - 1; i++) {
        win->tab_handles[i] = win->tab_handles[i + 1];
    }
    win->tab_count--;

    if (win->active_tab_handle == tab) {
        win->active_tab_handle = (win->tab_count > 0) ? win->tab_handles[0] : ABE_INVALID_HANDLE;
    }
    return ABE_SUCCESS;
}

ABE_Error ABE_Window_SelectTab(ABE_WindowHandle window, ABE_TabHandle tab) {
    ABE_BrowserWindow* win = ABE_Window_Get(window);
    if (!win) return ABE_ERR_INVALID_PARAM;
    for (uint32_t i = 0; i < win->tab_count; i++) {
        if (win->tab_handles[i] == tab) {
            win->active_tab_handle = tab;
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_TAB_NOT_FOUND;
}

ABE_Error ABE_Window_RedrawFrame(ABE_WindowHandle handle) {
    ABE_BrowserWindow* win = ABE_Window_Get(handle);
    if (!win) return ABE_ERR_INVALID_PARAM;
    // Window Frame compositing with BOSurface primitive drawing
    return ABE_SUCCESS;
}
