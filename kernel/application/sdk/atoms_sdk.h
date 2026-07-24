#ifndef ATOMS_SDK_H
#define ATOMS_SDK_H

#include <stdint.h>
#include <stdbool.h>

#include "kernel/application/app_manager/app_manager.h"
#include "kernel/application/permissions/app_permissions.h"
#include "kernel/application/loader/app_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Native SDK Versioning
// ============================================================
#define ATOMS_SDK_VERSION_MAJOR 1
#define ATOMS_SDK_VERSION_MINOR 0
#define ATOMS_SDK_VERSION_PATCH 0
#define ATOMS_SDK_NAME          "ATOMS OS Native C SDK v1.0"

// ============================================================
// Unified Application Entry Point Macro
// ============================================================
#define ATOMS_NATIVE_APP_MAIN(app_name_str, app_ver_str) \
    static uint32_t s_atoms_app_id = 0; \
    static uint32_t s_atoms_main_window_id = 0;

#ifdef __cplusplus
}
#endif

#endif // ATOMS_SDK_H
