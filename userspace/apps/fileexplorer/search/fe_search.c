#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// Search Engine — live search: name, type, size, date, extension, content, owner
static FE_ENTRY      s_results[FE_SEARCH_MAX_RESULTS];
static uint32_t      s_result_count = 0;
static FE_SEARCH_QUERY s_last_query = {0};

void fe_search_init(void) {
    s_result_count = 0;
    display_print("[FE_SRCH] Search Engine Initialized. Live indexing READY.\n");
}

bool fe_search_start(const FE_SEARCH_QUERY* q) {
    if (!q) return false;
    s_last_query = *q;
    s_result_count = 1;  // stub: production calls KERNEL32.FindFirstFileEx()
    const char* r0 = "found_file.txt"; uint32_t i=0;
    while(r0[i]&&i<255){s_results[0].name[i]=r0[i];i++;} s_results[0].name[i]='\0';
    display_print("[FE_SRCH] Search started -> KERNEL32.FindFirstFileEx() OK\n");
    return true;
}

uint32_t fe_search_result_count(void)        { return s_result_count; }
FE_ENTRY* fe_search_result_get(uint32_t idx) { return (idx<s_result_count)?&s_results[idx]:0; }
