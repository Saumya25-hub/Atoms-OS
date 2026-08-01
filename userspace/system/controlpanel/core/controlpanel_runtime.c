#include "../include/controlpanel_api.h"
#include "kernel/drivers/display/display.h"

static bool g_controlpanel_initialized = false;

int32_t ControlPanelInitialize(void) {
    if (g_controlpanel_initialized) return 0;
    display_print("[CONTROLPANEL] Initializing ControlPanel.BOSX V1.0 System Configuration Center...\n");
    g_controlpanel_initialized = true;
    display_print("[CONTROLPANEL] ControlPanel.BOSX V1.0 Initialized Successfully.\n");
    return 0;
}

void ControlPanelShutdown(void) {
    display_print("[CONTROLPANEL] Shutting down ControlPanel Configuration Center...\n");
    g_controlpanel_initialized = false;
}
