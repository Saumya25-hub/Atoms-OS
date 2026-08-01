#include "../include/bfs_api.h"
#include "kernel/core/lib/include/string.h"

static BFS_ItemEntry s_recent_items[BFS_MAX_RECENT];
static uint32_t      s_recent_count = 0;

int32_t BFS_GetRecent(BFS_ItemEntry* out_items, uint32_t* out_count) {
    if (!out_items || !out_count) return -1;
    if (s_recent_count == 0) {
        strcpy(s_recent_items[0].name, "Document.txt");
        strcpy(s_recent_items[0].path, "/DOCS/Document.txt");
        s_recent_items[0].is_directory = false;
        s_recent_items[0].icon_id = 1;
        s_recent_count = 1;
    }
    for (uint32_t i = 0; i < s_recent_count; i++) {
        out_items[i] = s_recent_items[i];
    }
    *out_count = s_recent_count;
    return 0;
}
