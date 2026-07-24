#include "app_installer.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

static ATOMS_InstalledApp g_installed_db[ATOMS_MAX_INSTALLED_APPS];
static uint32_t           g_next_installer_id = 500;

static void str_copy_limit(char* dest, const char* src, uint32_t limit) {
    if (!dest || !src || limit == 0) return;
    uint32_t i = 0;
    while (src[i] && i < limit - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void ATOMS_AppInstaller_Init(void) {
    for (uint32_t i = 0; i < ATOMS_MAX_INSTALLED_APPS; i++) {
        g_installed_db[i].app_id = 0;
        g_installed_db[i].installed = false;
    }
    bwe_log("INFO", "ATOMS Application Installer Database Initialized");
}

bool ATOMS_AppInstaller_Install(const char* name, const char* version, const char* publisher, const char* path, uint32_t capabilities, uint32_t* out_app_id) {
    if (!name || !path) return false;

    for (uint32_t i = 0; i < ATOMS_MAX_INSTALLED_APPS; i++) {
        if (!g_installed_db[i].installed) {
            uint32_t id = g_next_installer_id++;
            g_installed_db[i].app_id = id;
            str_copy_limit(g_installed_db[i].name, name, sizeof(g_installed_db[i].name));
            str_copy_limit(g_installed_db[i].version, version ? version : "1.0", sizeof(g_installed_db[i].version));
            str_copy_limit(g_installed_db[i].publisher, publisher ? publisher : "ATOMS System", sizeof(g_installed_db[i].publisher));
            str_copy_limit(g_installed_db[i].install_path, path, sizeof(g_installed_db[i].install_path));
            str_copy_limit(g_installed_db[i].checksum_sha256, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", 65);
            g_installed_db[i].capabilities_mask = capabilities;
            g_installed_db[i].sll_count = 0;
            g_installed_db[i].installed = true;

            if (out_app_id) *out_app_id = id;
            return true;
        }
    }
    return false;
}

bool ATOMS_AppInstaller_Uninstall(uint32_t app_id) {
    for (uint32_t i = 0; i < ATOMS_MAX_INSTALLED_APPS; i++) {
        if (g_installed_db[i].installed && g_installed_db[i].app_id == app_id) {
            g_installed_db[i].installed = false;
            g_installed_db[i].app_id = 0;
            return true;
        }
    }
    return false;
}

ATOMS_InstalledApp* ATOMS_AppInstaller_GetApp(uint32_t app_id) {
    if (app_id == 0) return 0;
    for (uint32_t i = 0; i < ATOMS_MAX_INSTALLED_APPS; i++) {
        if (g_installed_db[i].installed && g_installed_db[i].app_id == app_id) {
            return &g_installed_db[i];
        }
    }
    return 0;
}

uint32_t ATOMS_AppInstaller_GetInstalledCount(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < ATOMS_MAX_INSTALLED_APPS; i++) {
        if (g_installed_db[i].installed) count++;
    }
    return count;
}
