#include "../include/bar_api.h"

static BARWindowID g_clipboard_owner = 0;

int32_t BAR_OpenClipboard(BARWindowID win_id) {
    if (win_id == 0) return -1;
    g_clipboard_owner = win_id;
    return 0;
}

int32_t BAR_CloseClipboard(void) {
    g_clipboard_owner = 0;
    return 0;
}
