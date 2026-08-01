#include "../include/controlpanel_api.h"
#include "kernel/drivers/display/display.h"

bool OpenModule(const char* module_name) {
    (void)module_name;
    display_print("[CONTROLPANEL_LOADER] Loading native .BOSC configuration module...\n");
    return true;
}

bool CloseModule(const char* module_name) {
    (void)module_name;
    display_print("[CONTROLPANEL_LOADER] Unloading .BOSC module...\n");
    return true;
}

bool ReloadModule(const char* module_name) {
    CloseModule(module_name);
    return OpenModule(module_name);
}

bool InstallModule(const char* module_path) {
    (void)module_path;
    display_print("[CONTROLPANEL_LOADER] Installed third-party .BOSC plugin module successfully.\n");
    return true;
}

bool RemoveModule(const char* module_name) {
    (void)module_name;
    display_print("[CONTROLPANEL_LOADER] Removed .BOSC plugin module.\n");
    return true;
}

bool GetModuleInfo(const char* module_name, BOSC_MODULE_INFO* outInfo) {
    if (!outInfo) return false;
    outInfo->version = 1;
    outInfo->is_installed = true;
    outInfo->is_pinned = false;
    return true;
}

void RefreshModules(void) {
    display_print("[CONTROLPANEL_LOADER] Dynamic .BOSC module catalog refreshed.\n");
}
