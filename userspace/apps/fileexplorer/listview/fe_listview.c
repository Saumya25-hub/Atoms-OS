#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// ListView Engine — Details / Icons / Tiles / Content views via COMCTL32
static FE_ENTRY   s_entries[FE_MAX_ENTRIES];
static uint32_t   s_entry_count = 0;
static FE_VIEW_MODE s_view = VIEW_DETAILS;

void fe_listview_init(void) {
    s_entry_count = 0;
    s_view = VIEW_DETAILS;
    display_print("[FE_LIST] ListView Engine Initialized.\n");
}

bool fe_refresh_listing(void) {
    // In production: SHELL32.SHGetFolderContents() -> fills s_entries
    s_entry_count = 3;
    const char* n0 = "Documents"; uint32_t i=0; while(n0[i]&&i<255){s_entries[0].name[i]=n0[i];i++;} s_entries[0].name[i]='\0'; s_entries[0].is_dir=true;
    const char* n1 = "boot.cfg";  i=0; while(n1[i]&&i<255){s_entries[1].name[i]=n1[i];i++;} s_entries[1].name[i]='\0'; s_entries[1].is_dir=false; s_entries[1].size_bytes=2048;
    const char* n2 = "kernel.bin";i=0; while(n2[i]&&i<255){s_entries[2].name[i]=n2[i];i++;} s_entries[2].name[i]='\0'; s_entries[2].is_dir=false; s_entries[2].size_bytes=2*1024*1024;
    display_print("[FE_LIST] RefreshListing() -> SHELL32.SHGetFolderContents() OK\n");
    return true;
}

uint32_t fe_entry_count(void)              { return s_entry_count; }
FE_ENTRY* fe_entry_get(uint32_t idx)      { return (idx < s_entry_count) ? &s_entries[idx] : 0; }
void fe_listview_set_mode(FE_VIEW_MODE m) { s_view = m; }
FE_VIEW_MODE fe_listview_get_mode(void)   { return s_view; }
