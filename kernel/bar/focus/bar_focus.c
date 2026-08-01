#include "../include/bar_api.h"

static BARWindowID g_focused_window = 0;

int32_t BAR_SetFocus(BARWindowID win_id) {
    g_focused_window = win_id;
    return 0;
}

BARWindowID BAR_GetFocus(void) {
    return g_focused_window;
}
