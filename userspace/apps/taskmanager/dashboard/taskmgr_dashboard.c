#include "../include/taskmgr_api.h"
#include "kernel/drivers/display/display.h"

// Dashboard Engine — live overview panel
void taskmgr_dashboard_init(void) { display_print("[TASKMGR_DASH] Dashboard Engine Initialized.\n"); }
bool RefreshPerformance(void) {
    extern bool taskmgr_performance_refresh(void);
    return taskmgr_performance_refresh();
}
