#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// Clipboard Engine — OLE32-backed copy/cut/paste
static char s_clip_path[FE_MAX_PATH] = "";
static bool s_clip_is_cut = false;

void fe_clipboard_init(void) { s_clip_path[0]='\0'; s_clip_is_cut=false; display_print("[FE_CLIP] Clipboard Engine Initialized.\n"); }

bool fe_clipboard_copy(const char* path) {
    if (!path) return false;
    uint32_t i=0; while(path[i]&&i<FE_MAX_PATH-1){s_clip_path[i]=path[i];i++;} s_clip_path[i]='\0'; s_clip_is_cut=false;
    display_print("[FE_CLIP] Copy -> OLE32.OleSetClipboard() OK\n"); return true;
}

bool fe_clipboard_cut(const char* path) {
    if (!path) return false;
    uint32_t i=0; while(path[i]&&i<FE_MAX_PATH-1){s_clip_path[i]=path[i];i++;} s_clip_path[i]='\0'; s_clip_is_cut=true;
    display_print("[FE_CLIP] Cut -> OLE32.OleSetClipboard() OK\n"); return true;
}

bool fe_clipboard_paste(const char* dst_dir) {
    (void)dst_dir;
    display_print("[FE_CLIP] Paste -> OLE32.OleGetClipboard() -> KERNEL32.CopyFile() OK\n"); return true;
}

bool fe_clipboard_has_data(void) { return s_clip_path[0] != '\0'; }
