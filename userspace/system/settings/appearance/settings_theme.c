#include "../include/settings_api.h"
#include "kernel/drivers/display/display.h"

bool OpenAppearance(void) {
    display_print("[SETTINGS_THEME] Themes & Appearance Settings Loaded.\n");
    return true;
}
