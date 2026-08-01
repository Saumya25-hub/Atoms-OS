#include "../include/bar_api.h"

static bool g_session_active = false;
static uint32_t g_active_session_id = 0;

int32_t bar_session_init(void) {
    g_active_session_id = 1001;
    g_session_active = true;
    return 0;
}

uint32_t bar_get_active_session_id(void) {
    return g_active_session_id;
}
