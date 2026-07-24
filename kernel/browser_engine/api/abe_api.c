#include "abe_api.h"
#include "../core/abe_core.h"
#include "kernel/core/lib/include/string.h"

ABE_Error ABE_Initialize(const ABE_Config* config) {
    return ABE_Core_Initialize(config);
}

ABE_Error ABE_Shutdown(void) {
    return ABE_Core_Shutdown();
}

bool ABE_IsInitialized(void) {
    return ABE_Core_IsInitialized();
}

void ABE_GetDefaultConfig(ABE_Config* out_config) {
    ABE_Config_SetDefaults(out_config);
}

ABE_Error ABE_CreateWindow(const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height, ABE_WindowHandle* out_window) {
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    return ABE_Window_Create(title, x, y, width, height, out_window);
}

ABE_Error ABE_DestroyWindow(ABE_WindowHandle window) {
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    return ABE_Window_Destroy(window);
}

ABE_Error ABE_ResizeWindow(ABE_WindowHandle window, uint32_t width, uint32_t height) {
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    return ABE_Window_Resize(window, width, height);
}

ABE_Error ABE_SetWindowFullscreen(ABE_WindowHandle window, bool fullscreen) {
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    return ABE_Window_SetFullscreen(window, fullscreen);
}

ABE_Error ABE_GetWindowInfo(ABE_WindowHandle window, ABE_WindowInfo* out_info) {
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    if (!out_info) return ABE_ERR_INVALID_PARAM;
    ABE_BrowserWindow* win = ABE_Window_Get(window);
    if (!win) return ABE_ERR_WINDOW_FAILED;

    memset(out_info, 0, sizeof(ABE_WindowInfo));
    out_info->handle = win->handle;
    out_info->bosurface_id = win->bosurface_id;
    out_info->x = win->x;
    out_info->y = win->y;
    out_info->width = win->width;
    out_info->height = win->height;
    out_info->is_fullscreen = win->is_fullscreen;
    out_info->tab_count = win->tab_count;
    out_info->active_tab = win->active_tab_handle;
    return ABE_SUCCESS;
}

ABE_Error ABE_CreateTab(ABE_WindowHandle window, const char* initial_url, ABE_TabHandle* out_tab) {
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    return ABE_Tab_Create(window, initial_url, out_tab);
}

ABE_Error ABE_CloseTab(ABE_WindowHandle window, ABE_TabHandle tab) {
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    return ABE_Tab_Close(window, tab);
}

ABE_Error ABE_SelectTab(ABE_WindowHandle window, ABE_TabHandle tab) {
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    return ABE_Tab_Select(window, tab);
}

ABE_Error ABE_GetActiveTab(ABE_WindowHandle window, ABE_TabHandle* out_tab) {
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    if (!out_tab) return ABE_ERR_INVALID_PARAM;
    ABE_BrowserWindow* win = ABE_Window_Get(window);
    if (!win) return ABE_ERR_WINDOW_FAILED;
    if (win->active_tab_handle == ABE_INVALID_HANDLE) return ABE_ERR_TAB_NOT_FOUND;
    *out_tab = win->active_tab_handle;
    return ABE_SUCCESS;
}

ABE_Error ABE_GetTabInfo(ABE_TabHandle tab, ABE_TabInfo* out_info) {
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    if (!out_info) return ABE_ERR_INVALID_PARAM;
    ABE_TabNode* node = ABE_Tab_Get(tab);
    if (!node) return ABE_ERR_TAB_NOT_FOUND;

    memset(out_info, 0, sizeof(ABE_TabInfo));
    out_info->handle = node->handle;
    out_info->window_handle = node->owner_window;
    strncpy(out_info->title, node->title, ABE_MAX_TITLE_LEN - 1);
    strncpy(out_info->url, node->current_url, ABE_MAX_URL_LEN - 1);
    out_info->is_active = node->is_active_in_window;
    out_info->state = (uint32_t)node->state;
    out_info->load_progress = (node->history) ? node->history->load_progress : 100;
    out_info->is_loading = (node->state == TAB_STATE_LOADING);
    return ABE_SUCCESS;
}

ABE_Error ABE_LoadURL(ABE_WindowHandle window, ABE_TabHandle tab, const char* url) {
    (void)window;
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    return ABE_Tab_LoadURL(tab, url);
}

ABE_Error ABE_Reload(ABE_WindowHandle window, ABE_TabHandle tab) {
    (void)window;
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    return ABE_Tab_Reload(tab);
}

ABE_Error ABE_Stop(ABE_WindowHandle window, ABE_TabHandle tab) {
    (void)window;
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    return ABE_Tab_Stop(tab);
}

ABE_Error ABE_GoBack(ABE_WindowHandle window, ABE_TabHandle tab) {
    (void)window;
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    return ABE_Tab_GoBack(tab);
}

ABE_Error ABE_GoForward(ABE_WindowHandle window, ABE_TabHandle tab) {
    (void)window;
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    return ABE_Tab_GoForward(tab);
}

bool ABE_CanGoBack(ABE_TabHandle tab) {
    if (!ABE_Core_IsInitialized()) return false;
    ABE_TabNode* node = ABE_Tab_Get(tab);
    if (!node || !node->history) return false;
    return ABE_Navigation_CanGoBack(node->history);
}

bool ABE_CanGoForward(ABE_TabHandle tab) {
    if (!ABE_Core_IsInitialized()) return false;
    ABE_TabNode* node = ABE_Tab_Get(tab);
    if (!node || !node->history) return false;
    return ABE_Navigation_CanGoForward(node->history);
}

ABE_Error ABE_GetDiagnosticsMetrics(ABE_DiagnosticsMetrics* out_metrics) {
    if (!ABE_Core_IsInitialized()) return ABE_ERR_NOT_INITIALIZED;
    if (!out_metrics) return ABE_ERR_INVALID_PARAM;
    *out_metrics = ABE_Diagnostics_GetMetrics();
    return ABE_SUCCESS;
}

void ABE_DumpDiagnostics(char* buffer, size_t max_len) {
    if (!ABE_Core_IsInitialized()) return;
    ABE_DumpMemoryDiagnostics(buffer, max_len);
}
