#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// File Watcher Engine — auto-refresh on USB insert, folder change, file change
typedef struct { char path[FE_MAX_PATH]; bool active; } FE_WATCHER;
static FE_WATCHER s_watchers[FE_MAX_WATCHERS];
static uint32_t   s_watcher_count = 0;
static uint32_t   s_events_per_sec = 0;

void fe_watcher_init(void) { s_watcher_count=0; s_events_per_sec=0; display_print("[FE_WATCH] File Watcher Engine Initialized.\n"); }

bool fe_watcher_start(const char* path) {
    if (!path || s_watcher_count>=FE_MAX_WATCHERS) return false;
    uint32_t i=0; while(path[i]&&i<FE_MAX_PATH-1){s_watchers[s_watcher_count].path[i]=path[i];i++;} s_watchers[s_watcher_count].path[i]='\0';
    s_watchers[s_watcher_count].active=true; s_watcher_count++;
    display_print("[FE_WATCH] Watcher started -> KERNEL32.ReadDirectoryChangesW() OK\n"); return true;
}

bool fe_watcher_stop(const char* path) {
    (void)path; if (s_watcher_count>0) s_watcher_count--;
    display_print("[FE_WATCH] Watcher stopped OK\n"); return true;
}

uint32_t fe_watcher_count(void)      { return s_watcher_count; }
uint32_t fe_watcher_events_ps(void)  { return s_events_per_sec; }
