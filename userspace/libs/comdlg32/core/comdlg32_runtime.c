#include "../include/comdlg32_api.h"
#include "kernel/drivers/display/display.h"

static bool g_comdlg32_initialized = false;

int32_t COMDLG32_Init(void) {
    if (g_comdlg32_initialized) return 0;
    g_comdlg32_initialized = true;
    display_print("[COMDLG32] COMDLG32.sll Common Dialog Framework V1.0 Initialized\n");
    return 0;
}

int32_t COMDLG32_Shutdown(void) {
    if (!g_comdlg32_initialized) return 0;
    g_comdlg32_initialized = false;
    display_print("[COMDLG32] COMDLG32.sll Common Dialog Framework Shutdown Cleanly\n");
    return 0;
}

DWORD CommDlgExtendedError(void) {
    return 0;
}
