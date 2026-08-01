#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

static char s_recent[FE_MAX_RECENT][FE_MAX_PATH];
static uint32_t s_recent_count = 0;

void fe_recent_init(void) { s_recent_count=0; display_print("[FE_REC] Recent Files Engine Initialized.\n"); }

void fe_recent_push(const char* path) {
    if (!path || s_recent_count>=FE_MAX_RECENT) return;
    uint32_t i=0; while(path[i]&&i<FE_MAX_PATH-1){s_recent[s_recent_count][i]=path[i];i++;} s_recent[s_recent_count][i]='\0'; s_recent_count++;
}

uint32_t fe_recent_count(void)            { return s_recent_count; }
const char* fe_recent_get(uint32_t idx)   { return (idx<s_recent_count)?s_recent[idx]:""; }
