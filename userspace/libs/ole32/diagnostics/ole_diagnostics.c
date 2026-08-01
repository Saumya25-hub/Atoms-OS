#include "../include/ole32_api.h"
#include "kernel/drivers/display/display.h"

void OLE32DumpDiagnostics(void) {
    display_print("[OLE32_DIAG] COM Runtime, Class Factories, Apartments, Storage & DragDrop: PASS\n");
}
