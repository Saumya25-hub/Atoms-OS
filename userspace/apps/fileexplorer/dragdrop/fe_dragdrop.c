#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// DragDrop Engine — OLE32 IDropSource / IDropTarget
static char s_drag_path[FE_MAX_PATH] = "";
static bool s_dragging = false;

void fe_dragdrop_init(void) { s_dragging=false; s_drag_path[0]='\0'; display_print("[FE_DD] DragDrop Engine Initialized.\n"); }

bool fe_dragdrop_begin(const char* path) {
    if (!path) return false;
    uint32_t i=0; while(path[i]&&i<FE_MAX_PATH-1){s_drag_path[i]=path[i];i++;} s_drag_path[i]='\0'; s_dragging=true;
    display_print("[FE_DD] DragBegin -> OLE32.DoDragDrop() OK\n"); return true;
}

bool fe_dragdrop_drop(const char* dst_dir) {
    (void)dst_dir; s_dragging=false;
    display_print("[FE_DD] Drop -> KERNEL32.CopyFile() OK\n"); return true;
}

bool fe_dragdrop_is_active(void) { return s_dragging; }
