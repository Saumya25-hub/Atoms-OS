#include "../include/bar_api.h"

static bool g_drag_active = false;

int32_t BAR_BeginDrag(BARWindowID win_id, const char* mime_type, const void* data, size_t size) {
    if (win_id == 0 || !mime_type || !data || size == 0) return -1;
    g_drag_active = true;
    return 0;
}

int32_t BAR_EndDrag(void) {
    g_drag_active = false;
    return 0;
}
