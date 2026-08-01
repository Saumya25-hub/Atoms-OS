#include "../include/ole32_api.h"
#include "kernel/drivers/display/display.h"

static bool g_ole32_initialized = false;

int32_t Ole32Initialize(void) {
    if (g_ole32_initialized) return 0;
    display_print("[OLE32] Initializing OLE32.sll V1.0 COM Runtime & Component Infrastructure...\n");
    g_ole32_initialized = true;
    display_print("[OLE32] OLE32.sll V1.0 COM Runtime Initialized Successfully.\n");
    return 0;
}
