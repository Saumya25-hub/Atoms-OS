#include "../include/bfs_api.h"
#include "kernel/core/lib/include/string.h"

int32_t BFS_Search(const char* pattern, BFS_ItemEntry** out_results, uint32_t* out_count) {
    if (!pattern || !out_results || !out_count) return -1;
    *out_results = NULL;
    *out_count = 0;
    return 0;
}
