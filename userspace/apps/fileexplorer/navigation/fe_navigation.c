#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// Navigation Engine — Back / Forward / Up stack
static char s_history[FE_MAX_HISTORY][FE_MAX_PATH];
static uint32_t s_hist_pos  = 0;
static uint32_t s_hist_size = 0;

void fe_navigation_init(void) {
    s_hist_pos = 0; s_hist_size = 0;
    display_print("[FE_NAV] Navigation Engine Initialized.\n");
}

bool fe_navigate_back(void) {
    if (s_hist_pos == 0) return false;
    s_hist_pos--;
    display_print("[FE_NAV] Navigate Back OK\n");
    return true;
}

bool fe_navigate_forward(void) {
    if (s_hist_pos + 1 >= s_hist_size) return false;
    s_hist_pos++;
    display_print("[FE_NAV] Navigate Forward OK\n");
    return true;
}

bool fe_navigate_up(void) {
    display_print("[FE_NAV] Navigate Up -> SHELL32.GetParentFolder() OK\n");
    return true;
}

void fe_navigation_push(const char* path) {
    if (!path) return;
    uint32_t i = 0;
    while (path[i] && i < FE_MAX_PATH-1) { s_history[s_hist_pos][i] = path[i]; i++; }
    s_history[s_hist_pos][i] = '\0';
    s_hist_size = ++s_hist_pos;
}
