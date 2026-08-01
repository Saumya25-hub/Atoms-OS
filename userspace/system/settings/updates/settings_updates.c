#include "../include/settings_api.h"
#include "kernel/drivers/display/display.h"

bool OpenUpdates(void) {
    display_print("[SETTINGS_UPDATES] System Updates & Version Center Loaded.\n");
    return true;
}
