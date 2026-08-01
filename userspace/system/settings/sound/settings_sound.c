#include "../include/settings_api.h"
#include "kernel/drivers/display/display.h"

bool OpenSound(void) {
    display_print("[SETTINGS_SOUND] Audio & Sound Settings Loaded.\n");
    return true;
}
