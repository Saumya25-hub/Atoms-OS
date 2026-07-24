#include "browser_tabs.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

static ATRIX_BrowserTab g_tabs[ATRIX_MAX_TABS_LIMIT];
static uint32_t        g_active_tab_id = 1;
static uint32_t        g_next_tab_id = 1;

static void str_copy_limit(char* dest, const char* src, uint32_t limit) {
    if (!dest || !src || limit == 0) return;
    uint32_t i = 0;
    while (src[i] && i < limit - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void ATRIX_TabManager_Init(void) {
    for (uint32_t i = 0; i < ATRIX_MAX_TABS_LIMIT; i++) {
        g_tabs[i].tab_id = 0;
        g_tabs[i].is_active = false;
    }
    ATRIX_TabManager_CreateTab("atrix://newtab");
}

ATRIX_BrowserTab* ATRIX_TabManager_CreateTab(const char* initial_url) {
    for (uint32_t i = 0; i < ATRIX_MAX_TABS_LIMIT; i++) {
        if (g_tabs[i].tab_id == 0) {
            g_tabs[i].tab_id = g_next_tab_id++;
            str_copy_limit(g_tabs[i].title, "New Tab", sizeof(g_tabs[i].title));
            str_copy_limit(g_tabs[i].url, initial_url ? initial_url : "about:blank", sizeof(g_tabs[i].url));
            g_tabs[i].is_active = true;
            g_tabs[i].is_loading = false;
            g_active_tab_id = g_tabs[i].tab_id;
            return &g_tabs[i];
        }
    }
    return 0;
}

bool ATRIX_TabManager_CloseTab(uint32_t tab_id) {
    for (uint32_t i = 0; i < ATRIX_MAX_TABS_LIMIT; i++) {
        if (g_tabs[i].tab_id == tab_id) {
            g_tabs[i].tab_id = 0;
            g_tabs[i].is_active = false;
            return true;
        }
    }
    return false;
}

ATRIX_BrowserTab* ATRIX_TabManager_GetActiveTab(void) {
    for (uint32_t i = 0; i < ATRIX_MAX_TABS_LIMIT; i++) {
        if (g_tabs[i].tab_id == g_active_tab_id) {
            return &g_tabs[i];
        }
    }
    return 0;
}

uint32_t ATRIX_TabManager_GetTabCount(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < ATRIX_MAX_TABS_LIMIT; i++) {
        if (g_tabs[i].tab_id != 0) count++;
    }
    return count;
}
