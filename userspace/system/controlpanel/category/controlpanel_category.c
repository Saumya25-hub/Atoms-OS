#include "../include/controlpanel_api.h"
#include "kernel/drivers/display/display.h"

void ControlPanelGetCategories(void) {
    display_print("[CONTROLPANEL_CAT] Categories: System, Hardware, Personalization, Network, Security, Apps, User Accounts\n");
}
