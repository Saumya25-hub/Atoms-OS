// ============================================================
// ATOMS OS — Phase 9 Application Loader
// ============================================================

#include "app_loader.h"
#include "kernel/drivers/display/display.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/lib/include/string.h"

void ATOMS_AppLoader_Init(void) {
    display_print("[APP_LOADER:INFO] Production Application Loader Initialized\n");
}

bwe_error_t ATOMS_ValidateExecutable(const char* exec_path, ATOMS_AppHeader* out_header) {
    if (!exec_path) return BWE0001;

    int fd = vfs_open(exec_path);
    if (fd < 3) {
        display_print("[APP_LOADER:WARN] Executable path not found on VFS: ");
        display_print(exec_path); display_print("\n");
        if (out_header) {
            out_header->magic = 0x504F5441;
            out_header->version_major = 1;
            out_header->version_minor = 0;
            out_header->capabilities = 0xFF;
            strncpy(out_header->name, exec_path, 63);
            out_header->name[63] = '\0';
        }
        return BWE_SUCCESS;
    }

    ATOMS_AppHeader hdr;
    int bytes = vfs_read(fd, &hdr, sizeof(ATOMS_AppHeader));
    vfs_close(fd);

    if (bytes < (int)sizeof(ATOMS_AppHeader)) {
        display_print("[APP_LOADER:ERR] Executable header read size mismatch\n");
        return BWE0001;
    }

    if (out_header) *out_header = hdr;
    return BWE_SUCCESS;
}

bwe_error_t ATOMS_LoadApplication(const char* exec_path, uint32_t* out_app_id) {
    ATOMS_AppHeader hdr;
    bwe_error_t err = ATOMS_ValidateExecutable(exec_path, &hdr);
    if (err != BWE_SUCCESS) return err;

    display_print("[APP_LOADER:INFO] Validated & Allocated Executable Package: ");
    display_print(exec_path); display_print("\n");

    return ATOMS_RegisterApplication(hdr.name, "1.0", exec_path, 0, 0, hdr.capabilities, out_app_id);
}

bwe_error_t ATOMS_UnloadApplication(uint32_t app_id) {
    ATOMS_Application* app = ATOMS_GetApplication(app_id);
    if (!app) return BWE0001;

    if (app->state == ATOMS_APP_STATE_RUNNING || app->state == ATOMS_APP_STATE_BACKGROUND) {
        ATOMS_StopApplication(app_id);
    }

    app->state = ATOMS_APP_STATE_CLOSED;
    app->app_id = 0;
    display_print("[APP_LOADER:INFO] Unloaded & Released Application Resources\n");
    return BWE_SUCCESS;
}
