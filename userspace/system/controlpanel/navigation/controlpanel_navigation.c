#include "../include/controlpanel_api.h"
#include "kernel/drivers/display/display.h"

bool OpenSettingsPage(const char* page_name) {
    (void)page_name;
    display_print("[CONTROLPANEL_NAV] Displaying settings page...\n");
    return true;
}
