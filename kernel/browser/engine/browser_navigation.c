#include "browser_navigation.h"
#include "browser_url.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

static ATRIX_NavigationState g_nav_state;

static void str_copy_limit(char* dest, const char* src, uint32_t limit) {
    if (!dest || !src || limit == 0) return;
    uint32_t i = 0;
    while (src[i] && i < limit - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void ATRIX_Navigation_Init(void) {
    g_nav_state.current_index = -1;
    g_nav_state.total_entries = 0;
    g_nav_state.is_loading = false;
    g_nav_state.last_status_code = 200;

    bwe_log("INFO", "ATRIX Navigation Engine Initialized");
    ATRIX_Navigation_Navigate("atrix://newtab");
}

bool ATRIX_Navigation_Navigate(const char* url) {
    if (!url) return false;
    ATRIX_ParsedURL parsed = ATRIX_URL_Parse(url);
    if (!parsed.is_valid) return false;

    if (g_nav_state.current_index + 1 < ATRIX_NAV_HISTORY_MAX) {
        g_nav_state.current_index++;
        str_copy_limit(g_nav_state.history_urls[g_nav_state.current_index], parsed.raw_url, 128);
        g_nav_state.total_entries = g_nav_state.current_index + 1;
    }

    g_nav_state.is_loading = true;
    g_nav_state.last_status_code = 200;
    return true;
}

bool ATRIX_Navigation_Back(void) {
    if (g_nav_state.current_index > 0) {
        g_nav_state.current_index--;
        return true;
    }
    return false;
}

bool ATRIX_Navigation_Forward(void) {
    if (g_nav_state.current_index + 1 < g_nav_state.total_entries) {
        g_nav_state.current_index++;
        return true;
    }
    return false;
}

bool ATRIX_Navigation_Reload(void) {
    if (g_nav_state.current_index >= 0) {
        g_nav_state.is_loading = true;
        return true;
    }
    return false;
}

bool ATRIX_Navigation_Stop(void) {
    g_nav_state.is_loading = false;
    return true;
}

bool ATRIX_Navigation_Home(void) {
    return ATRIX_Navigation_Navigate("atrix://newtab");
}

ATRIX_NavigationState* ATRIX_Navigation_GetState(void) {
    return &g_nav_state;
}
