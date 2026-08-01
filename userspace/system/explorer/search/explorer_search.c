#include "../include/explorer_api.h"
#include "kernel/drivers/display/display.h"

void ExplorerSearch(const char* query) {
    (void)query;
    display_print("[EXPLORER_SEARCH] Desktop Search Query Executed.\n");
}
