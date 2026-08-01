#include "../include/settings_api.h"
#include "kernel/drivers/display/display.h"

bool OpenNotifications(void) {
    display_print("[SETTINGS_NOTIF] Notification Preferences & Alerts Settings Loaded.\n");
    return true;
}
