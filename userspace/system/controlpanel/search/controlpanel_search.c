#include "../include/controlpanel_api.h"
#include "kernel/drivers/display/display.h"

bool SearchModule(const char* query) {
    (void)query;
    display_print("[CONTROLPANEL_SEARCH] Executing instant settings search query...\n");
    return true;
}
