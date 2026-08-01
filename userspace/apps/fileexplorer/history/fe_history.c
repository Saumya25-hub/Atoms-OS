#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

static FE_HISTORY_ENTRY s_hist[FE_MAX_HISTORY];
static uint32_t         s_hist_count = 0;

void fe_history_init(void) { s_hist_count=0; display_print("[FE_HIST] Navigation History Engine Initialized.\n"); }

void fe_history_push(const char* path) {
    if (!path || s_hist_count>=FE_MAX_HISTORY) return;
    uint32_t i=0; while(path[i]&&i<FE_MAX_PATH-1){s_hist[s_hist_count].path[i]=path[i];i++;} s_hist[s_hist_count].path[i]='\0';
    s_hist[s_hist_count].timestamp = s_hist_count * 1000; s_hist_count++;
}

uint32_t fe_history_count(void)                 { return s_hist_count; }
FE_HISTORY_ENTRY* fe_history_get(uint32_t idx) { return (idx<s_hist_count)?&s_hist[idx]:0; }
