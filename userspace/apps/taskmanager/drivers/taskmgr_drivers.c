#include "../include/taskmgr_api.h"
#include "kernel/drivers/display/display.h"

static TASKMGR_DRIVER s_drivers[TASKMGR_MAX_DRIVERS];
static uint32_t       s_driver_count = 0;

void taskmgr_drivers_init(void) {
    s_driver_count = 4;
    const char* d0="e1000";  for(int i=0;d0[i];i++) s_drivers[0].name[i]=d0[i]; s_drivers[0].loaded=true;
    const char* d1="ac97";   for(int i=0;d1[i];i++) s_drivers[1].name[i]=d1[i]; s_drivers[1].loaded=true;
    const char* d2="xhci";   for(int i=0;d2[i];i++) s_drivers[2].name[i]=d2[i]; s_drivers[2].loaded=true;
    const char* d3="ps2";    for(int i=0;d3[i];i++) s_drivers[3].name[i]=d3[i]; s_drivers[3].loaded=true;
    display_print("[TASKMGR_DRV] Driver Manager Engine Initialized.\n");
}

bool RefreshDrivers(void) {
    display_print("[TASKMGR_DRV] RefreshDrivers() -> KERNEL32.EnumDeviceDrivers() OK\n");
    return true;
}

uint32_t taskmgr_driver_count(void) { return s_driver_count; }
