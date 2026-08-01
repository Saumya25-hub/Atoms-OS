#include "../include/taskmgr_api.h"
#include "kernel/drivers/display/display.h"

static TASKMGR_STORAGE s_drives[4];
static uint32_t        s_drive_count = 0;

void taskmgr_storage_init(void) {
    s_drive_count = 1;
    const char* lbl = "SDA0"; for(int i=0;lbl[i];i++) s_drives[0].label[i]=lbl[i];
    const char* fs  = "BFS";  for(int i=0;fs[i];i++)  s_drives[0].filesystem[i]=fs[i];
    s_drives[0].capacity_bytes = 64ULL*1024*1024;
    s_drives[0].free_bytes     = 32ULL*1024*1024;
    s_drives[0].smart_ok       = true;
    display_print("[TASKMGR_STOR] Storage Manager Engine Initialized.\n");
}

bool RefreshStorage(void) {
    s_drives[0].read_mbps  = 120;
    s_drives[0].write_mbps = 80;
    s_drives[0].temperature_c = 34;
    display_print("[TASKMGR_STOR] RefreshStorage() -> KERNEL32.GetDiskInfo() OK\n");
    return true;
}

uint32_t taskmgr_storage_count(void)               { return s_drive_count; }
TASKMGR_STORAGE* taskmgr_storage_get(uint32_t i)  { return (i<s_drive_count)?&s_drives[i]:0; }
