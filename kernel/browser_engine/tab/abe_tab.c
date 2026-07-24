#include "abe_tab.h"
#include "../window/abe_window.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static ABE_TabEngineManager g_tab_engine;
static uint32_t g_next_tab_id = 2000;
static bool g_tab_initialized = false;

ABE_Error ABE_TabEngine_Init(void) {
    memset(&g_tab_engine, 0, sizeof(ABE_TabEngineManager));
    g_tab_initialized = true;
    ABE_Log(ABE_LOG_INFO, "TAB", "ABE Tab Engine V1.0 initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_TabEngine_Shutdown(void) {
    if (!g_tab_initialized) return ABE_ERR_NOT_INITIALIZED;
    for (uint32_t i = 0; i < ABE_MAX_GLOBAL_TABS; i++) {
        if (g_tab_engine.tabs[i].state != TAB_STATE_UNINIT) {
            if (g_tab_engine.tabs[i].history) {
                ABE_Navigation_FreeHistory(g_tab_engine.tabs[i].history);
                g_tab_engine.tabs[i].history = NULL;
            }
            g_tab_engine.tabs[i].state = TAB_STATE_UNINIT;
        }
    }
    g_tab_initialized = false;
    ABE_Log(ABE_LOG_INFO, "TAB", "ABE Tab Engine shut down cleanly");
    return ABE_SUCCESS;
}

ABE_Error ABE_Tab_Create(ABE_WindowHandle window, const char* initial_url, ABE_TabHandle* out_handle) {
    if (!g_tab_initialized || !out_handle) return ABE_ERR_INVALID_PARAM;
    ABE_BrowserWindow* win = ABE_Window_Get(window);
    if (!win) return ABE_ERR_WINDOW_FAILED;

    uint32_t slot = ABE_INVALID_HANDLE;
    for (uint32_t i = 0; i < ABE_MAX_GLOBAL_TABS; i++) {
        if (g_tab_engine.tabs[i].state == TAB_STATE_UNINIT) {
            slot = i;
            break;
        }
    }

    if (slot == ABE_INVALID_HANDLE) return ABE_ERR_RESOURCE_EXHAUSTED;

    ABE_NavigationHistory* history = NULL;
    ABE_Error err = ABE_Navigation_CreateHistory(&history);
    if (err != ABE_SUCCESS) return err;

    ABE_TabNode* tab = &g_tab_engine.tabs[slot];
    memset(tab, 0, sizeof(ABE_TabNode));
    tab->handle = (g_next_tab_id++) | (slot << 16);
    tab->owner_window = window;
    tab->state = TAB_STATE_ACTIVE;
    tab->history = history;
    tab->last_active_timestamp = 1000;

    const char* url_to_load = (initial_url && initial_url[0] != '\0') ? initial_url : "about:home";
    ABE_Navigation_LoadURL(tab->history, url_to_load);

    const ABE_NavigationEntry* cur = ABE_Navigation_GetCurrentEntry(tab->history);
    if (cur) {
        strncpy(tab->current_url, cur->url.raw_url, ABE_MAX_URL_LEN - 1);
        strncpy(tab->title, cur->title, ABE_MAX_TITLE_LEN - 1);
    }

    ABE_Window_AttachTab(window, tab->handle);

    g_tab_engine.total_tabs_count++;
    *out_handle = tab->handle;

    ABE_Diag_RecordTabCreated();
    ABE_LogVal(ABE_LOG_INFO, "TAB", "Created Tab, Handle: ", tab->handle);
    return ABE_SUCCESS;
}

ABE_Error ABE_Tab_Close(ABE_WindowHandle window, ABE_TabHandle handle) {
    if (!g_tab_initialized || handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_TabNode* tab = ABE_Tab_Get(handle);
    if (!tab) return ABE_ERR_TAB_NOT_FOUND;

    ABE_Window_DetachTab(window, handle);

    if (tab->history) {
        ABE_Navigation_FreeHistory(tab->history);
        tab->history = NULL;
    }

    tab->state = TAB_STATE_UNINIT;
    if (g_tab_engine.total_tabs_count > 0) g_tab_engine.total_tabs_count--;

    ABE_Diag_RecordTabDestroyed();
    ABE_LogVal(ABE_LOG_INFO, "TAB", "Closed Tab, Handle: ", handle);
    return ABE_SUCCESS;
}

ABE_Error ABE_Tab_Select(ABE_WindowHandle window, ABE_TabHandle handle) {
    if (!g_tab_initialized) return ABE_ERR_NOT_INITIALIZED;
    ABE_TabNode* tab = ABE_Tab_Get(handle);
    if (!tab) return ABE_ERR_TAB_NOT_FOUND;

    ABE_Error err = ABE_Window_SelectTab(window, handle);
    if (err == ABE_SUCCESS) {
        tab->is_active_in_window = true;
        tab->last_active_timestamp = 2000;
        ABE_LogVal(ABE_LOG_INFO, "TAB", "Selected Active Tab, Handle: ", handle);
    }
    return err;
}

ABE_TabNode* ABE_Tab_Get(ABE_TabHandle handle) {
    if (!g_tab_initialized || handle == ABE_INVALID_HANDLE) return NULL;
    uint32_t slot = (handle >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_GLOBAL_TABS) return NULL;
    if (g_tab_engine.tabs[slot].handle == handle && g_tab_engine.tabs[slot].state != TAB_STATE_UNINIT) {
        return &g_tab_engine.tabs[slot];
    }
    return NULL;
}

ABE_Error ABE_Tab_LoadURL(ABE_TabHandle handle, const char* url) {
    ABE_TabNode* tab = ABE_Tab_Get(handle);
    if (!tab || !url) return ABE_ERR_INVALID_PARAM;

    tab->state = TAB_STATE_LOADING;
    ABE_Error err = ABE_Navigation_LoadURL(tab->history, url);
    if (err == ABE_SUCCESS) {
        const ABE_NavigationEntry* cur = ABE_Navigation_GetCurrentEntry(tab->history);
        if (cur) {
            strncpy(tab->current_url, cur->url.raw_url, ABE_MAX_URL_LEN - 1);
            strncpy(tab->title, cur->title, ABE_MAX_TITLE_LEN - 1);
        }
        tab->state = TAB_STATE_ACTIVE;
    } else {
        tab->state = TAB_STATE_CRASHED;
    }
    return err;
}

ABE_Error ABE_Tab_Reload(ABE_TabHandle handle) {
    ABE_TabNode* tab = ABE_Tab_Get(handle);
    if (!tab) return ABE_ERR_INVALID_PARAM;
    return ABE_Navigation_Reload(tab->history);
}

ABE_Error ABE_Tab_Stop(ABE_TabHandle handle) {
    ABE_TabNode* tab = ABE_Tab_Get(handle);
    if (!tab) return ABE_ERR_INVALID_PARAM;
    return ABE_Navigation_Stop(tab->history);
}

ABE_Error ABE_Tab_GoBack(ABE_TabHandle handle) {
    ABE_TabNode* tab = ABE_Tab_Get(handle);
    if (!tab) return ABE_ERR_INVALID_PARAM;
    ABE_Error err = ABE_Navigation_GoBack(tab->history);
    if (err == ABE_SUCCESS) {
        const ABE_NavigationEntry* cur = ABE_Navigation_GetCurrentEntry(tab->history);
        if (cur) {
            strncpy(tab->current_url, cur->url.raw_url, ABE_MAX_URL_LEN - 1);
            strncpy(tab->title, cur->title, ABE_MAX_TITLE_LEN - 1);
        }
    }
    return err;
}

ABE_Error ABE_Tab_GoForward(ABE_TabHandle handle) {
    ABE_TabNode* tab = ABE_Tab_Get(handle);
    if (!tab) return ABE_ERR_INVALID_PARAM;
    ABE_Error err = ABE_Navigation_GoForward(tab->history);
    if (err == ABE_SUCCESS) {
        const ABE_NavigationEntry* cur = ABE_Navigation_GetCurrentEntry(tab->history);
        if (cur) {
            strncpy(tab->current_url, cur->url.raw_url, ABE_MAX_URL_LEN - 1);
            strncpy(tab->title, cur->title, ABE_MAX_TITLE_LEN - 1);
        }
    }
    return err;
}
