#include "../include/bfs_api.h"
#include "kernel/core/lib/include/string.h"

int32_t BFS_GetQuickAccess(BFS_ItemEntry* out_items, uint32_t* out_count) {
    if (!out_items || !out_count) return -1;
    strcpy(out_items[0].name, "Desktop");
    strcpy(out_items[0].path, "virtual://Desktop");
    out_items[0].is_directory = true;
    out_items[0].icon_id = 100;
    *out_count = 1;
    return 0;
}
