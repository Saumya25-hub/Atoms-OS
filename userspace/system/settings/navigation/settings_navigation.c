#include "../include/settings_api.h"
#include "kernel/drivers/display/display.h"

bool OpenCategory(SETTINGS_CATEGORY_ID categoryId) {
    (void)categoryId;
    display_print("[SETTINGS_NAV] Navigated to requested category page.\n");
    return true;
}
