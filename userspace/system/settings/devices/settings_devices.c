#include "../include/settings_api.h"
#include "kernel/drivers/display/display.h"

bool OpenDevices(void) {
    display_print("[SETTINGS_DEVICES] Connected Devices & Peripherals Settings Loaded.\n");
    return true;
}
