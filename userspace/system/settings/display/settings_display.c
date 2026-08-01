#include "../include/settings_api.h"
#include "kernel/drivers/display/display.h"

bool OpenDisplay(void) {
    display_print("[SETTINGS_DISPLAY] Display Settings Page Loaded.\n");
    return true;
}
