#include "../include/settings_api.h"
#include "kernel/drivers/display/display.h"

bool OpenPersonalization(void) {
    display_print("[SETTINGS_PERS] Personalization Settings Page Loaded.\n");
    return true;
}
