#include "../include/explorer_api.h"
#include "kernel/drivers/display/display.h"

void ExplorerShowNotification(const char* title, const char* message) {
    (void)title; (void)message;
    display_print("[EXPLORER_TRAY] Toast Notification Displayed in System Tray.\n");
}
