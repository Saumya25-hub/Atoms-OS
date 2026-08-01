#include "../include/settings_api.h"
#include "kernel/drivers/display/display.h"

bool OpenPower(void) {
    display_print("[SETTINGS_POWER] Power & Battery Settings Page Loaded.\n");
    return true;
}
