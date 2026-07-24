#ifndef ATOMS_APP_INSTALLER_H
#define ATOMS_APP_INSTALLER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Application Installer Database (Phase 10)
// ============================================================

#define ATOMS_MAX_INSTALLED_APPS 64

typedef struct {
    uint32_t app_id;
    char     name[64];
    char     version[16];
    char     publisher[64];
    char     checksum_sha256[65];
    char     install_path[128];
    char     required_sll[4][32];
    uint32_t sll_count;
    uint32_t capabilities_mask;
    bool     installed;
} ATOMS_InstalledApp;

void               ATOMS_AppInstaller_Init(void);
bool               ATOMS_AppInstaller_Install(const char* name, const char* version, const char* publisher, const char* path, uint32_t capabilities, uint32_t* out_app_id);
bool               ATOMS_AppInstaller_Uninstall(uint32_t app_id);
ATOMS_InstalledApp* ATOMS_AppInstaller_GetApp(uint32_t app_id);
uint32_t           ATOMS_AppInstaller_GetInstalledCount(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_APP_INSTALLER_H
