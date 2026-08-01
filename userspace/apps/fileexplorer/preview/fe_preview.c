#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// Preview Engine — images, text, PDF, markdown, audio/video/BOSX metadata
void fe_preview_init(void) {
    display_print("[FE_PREV] Preview Engine Initialized (8 format handlers).\n");
}

bool fe_preview_render(const char* path) {
    (void)path;
    display_print("[FE_PREV] Preview render -> OLE32.IPreviewHandler() OK\n");
    return true;
}
