#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// Refresh Engine — manual and smart auto-refresh
void fe_refresh_init(void) { display_print("[FE_RFRSH] Refresh Engine Initialized.\n"); }

bool fe_force_refresh(void) {
    extern bool fe_refresh_listing(void);
    fe_refresh_listing();
    display_print("[FE_RFRSH] ForceRefresh() OK\n");
    return true;
}
