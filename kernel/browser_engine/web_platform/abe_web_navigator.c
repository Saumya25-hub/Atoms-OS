#include "abe_web_navigator.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_WebNavigatorInfo g_nav_info;
static bool g_web_nav_initialized = false;

ABE_Error ABE_WebNavigator_Init(void) {
    memset(&g_nav_info, 0, sizeof(ABE_WebNavigatorInfo));
    strcpy(g_nav_info.user_agent, "Mozilla/5.0 (ATOMS OS 1.0; x86_64) ATOMSBrowserEngine/1.0");
    strcpy(g_nav_info.platform, "ATOMS OS x86_64");
    strcpy(g_nav_info.language, "en-US");
    g_nav_info.hardware_concurrency = 8;
    g_nav_info.cookie_enabled = true;
    g_nav_info.on_line = true;

    g_web_nav_initialized = true;
    ABE_Log(ABE_LOG_INFO, "WEBNAVIGATOR", "ABE Navigator Subsystem initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_WebNavigator_Shutdown(void) {
    g_web_nav_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebNavigator_GetInfo(ABE_WebNavigatorInfo* out_info) {
    if (!g_web_nav_initialized || !out_info) return ABE_ERR_INVALID_PARAM;
    *out_info = g_nav_info;
    return ABE_SUCCESS;
}
