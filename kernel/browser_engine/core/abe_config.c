#include "abe_config.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_ConfigManager g_config_mgr;

void ABE_Config_SetDefaults(ABE_Config* config) {
    if (!config) return;
    memset(config, 0, sizeof(ABE_Config));
    strncpy(config->user_agent, "Mozilla/5.0 (ATOMS OS; ABE/1.0)", sizeof(config->user_agent) - 1);
    strncpy(config->default_home_url, "about:home", sizeof(config->default_home_url) - 1);
    config->max_tabs_per_window = 32;
    config->max_windows = 8;
    config->max_memory_pool_bytes = 64 * 1024 * 1024; // 64MB
    config->enable_diagnostics = true;
    config->enable_cache = true;
    config->log_level = 2; // INFO
}

ABE_Error ABE_Config_Init(const ABE_Config* config) {
    memset(&g_config_mgr, 0, sizeof(ABE_ConfigManager));
    if (config) {
        g_config_mgr.active_config = *config;
    } else {
        ABE_Config_SetDefaults(&g_config_mgr.active_config);
    }
    g_config_mgr.is_locked = false;
    g_config_mgr.config_revision = 1;
    ABE_Log(ABE_LOG_INFO, "CONFIG", "ABE Configuration Manager initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_Config_Shutdown(void) {
    memset(&g_config_mgr, 0, sizeof(ABE_ConfigManager));
    return ABE_SUCCESS;
}

const ABE_Config* ABE_Config_Get(void) {
    return &g_config_mgr.active_config;
}

ABE_Error ABE_Config_Update(const ABE_Config* new_config) {
    if (!new_config || g_config_mgr.is_locked) return ABE_ERR_INVALID_PARAM;
    g_config_mgr.active_config = *new_config;
    g_config_mgr.config_revision++;
    ABE_Log(ABE_LOG_INFO, "CONFIG", "ABE Configuration updated");
    return ABE_SUCCESS;
}
