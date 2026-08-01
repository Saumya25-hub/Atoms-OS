#include "../include/comctl32_api.h"
#include "kernel/drivers/display/display.h"

static bool g_comctl32_initialized = false;
static uint32_t g_control_handle_counter = 100;

int32_t COMCTL32_Init(void) {
    if (g_comctl32_initialized) return 0;
    g_comctl32_initialized = true;
    display_print("[COMCTL32] COMCTL32.sll Common Controls Runtime V1.0 Initialized\n");
    return 0;
}

int32_t COMCTL32_Shutdown(void) {
    if (!g_comctl32_initialized) return 0;
    g_comctl32_initialized = false;
    display_print("[COMCTL32] COMCTL32.sll Common Controls Runtime Shutdown Cleanly\n");
    return 0;
}

void InitCommonControls(void) {
    COMCTL32_Init();
}

BOOL InitCommonControlsEx(const INITCOMMONCONTROLSEX* picce) {
    (void)picce;
    COMCTL32_Init();
    return true;
}

HCONTROL comctl32_alloc_handle(void) {
    return (HCONTROL)(g_control_handle_counter++);
}

BOOL RefreshControl(HCONTROL hCtrl) { (void)hCtrl; return true; }
BOOL InvalidateControl(HCONTROL hCtrl) { (void)hCtrl; return true; }
BOOL UpdateControl(HCONTROL hCtrl) { (void)hCtrl; return true; }
BOOL DestroyControl(HCONTROL hCtrl) { return (hCtrl != 0); }
