#include "../include/controlpanel_api.h"
#include "kernel/drivers/display/display.h"

bool PinModule(const char* module_name) {
    (void)module_name;
    display_print("[CONTROLPANEL_FAV] Module pinned to favorites.\n");
    return true;
}

bool UnpinModule(const char* module_name) {
    (void)module_name;
    display_print("[CONTROLPANEL_FAV] Module unpinned from favorites.\n");
    return true;
}
