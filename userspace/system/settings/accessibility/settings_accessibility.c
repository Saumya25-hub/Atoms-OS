#include "../include/settings_api.h"
#include "kernel/drivers/display/display.h"

bool OpenAccessibility(void) {
    display_print("[SETTINGS_ACCESSIBILITY] Accessibility Options & Assistive Technology Page Loaded.\n");
    return true;
}
