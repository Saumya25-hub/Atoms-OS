// ============================================================
// ATOMS OS — Phase 9 Application Permissions & Security Foundation
// ============================================================

#include "app_permissions.h"
#include "kernel/application/app_manager/app_manager.h"
#include "kernel/drivers/display/display.h"

void ATOMS_Permissions_Init(void) {
    display_print("[APP_PERM:INFO] Application Security & Permission Architecture Initialized\n");
}

bool ATOMS_CheckCapability(uint32_t app_id, uint32_t required_capability) {
    ATOMS_Application* app = ATOMS_GetApplication(app_id);
    if (!app) return false;
    if (app->is_builtin) return true; // Built-in kernel apps have trusted full access

    return (app->capabilities_mask & required_capability) == required_capability;
}

void ATOMS_GrantCapability(uint32_t app_id, uint32_t capability) {
    ATOMS_Application* app = ATOMS_GetApplication(app_id);
    if (!app) return;
    app->capabilities_mask |= capability;
}

void ATOMS_RevokeCapability(uint32_t app_id, uint32_t capability) {
    ATOMS_Application* app = ATOMS_GetApplication(app_id);
    if (!app) return;
    app->capabilities_mask &= ~capability;
}
