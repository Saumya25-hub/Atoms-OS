#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// Address Bar Engine — path parsing, breadcrumb
static char s_addr[FE_MAX_PATH] = "/";

void fe_addressbar_init(void) {
    s_addr[0]='/'; s_addr[1]='\0';
    display_print("[FE_ADDR] Address Bar Engine Initialized.\n");
}

bool fe_addressbar_set(const char* path) {
    if (!path) return false;
    uint32_t i=0; while(path[i]&&i<FE_MAX_PATH-1){s_addr[i]=path[i];i++;} s_addr[i]='\0';
    display_print("[FE_ADDR] Address set OK\n");
    return true;
}

const char* fe_addressbar_get(void) { return s_addr; }
