#include "../include/settings_api.h"
#include "kernel/drivers/display/display.h"

bool OpenStorage(void) {
    display_print("[SETTINGS_STORAGE] Disk & Storage Management UI Loaded.\n");
    return true;
}
