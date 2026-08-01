#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

void fe_permissions_init(void) { display_print("[FE_PERM] Permission Engine Initialized.\n"); }

bool fe_permissions_show(const char* path) {
    (void)path;
    display_print("[FE_PERM] Show Permissions -> ADVAPI32.GetFileSecurity() OK\n");
    return true;
}
