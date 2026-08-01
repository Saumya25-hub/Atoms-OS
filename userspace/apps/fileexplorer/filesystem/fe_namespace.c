#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// Namespace Engine — virtual folders (This PC, Network, Recycle Bin, Quick Access)
static const char* s_virtual_roots[] = {
    "/", "BFS:", "FAT32:", "NET:", "RECYCLE:", "QUICKACCESS:", "VIRTUAL:"
};
static uint32_t s_virtual_count = 7;

void fe_namespace_init(void) {
    display_print("[FE_NS] Namespace Engine Initialized (7 virtual roots).\n");
}

uint32_t fe_namespace_root_count(void)        { return s_virtual_count; }
const char* fe_namespace_root_get(uint32_t i) { return (i<s_virtual_count)?s_virtual_roots[i]:""; }
