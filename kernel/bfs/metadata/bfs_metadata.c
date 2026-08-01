#include "../include/bfs_api.h"

int32_t BFS_QueryMetadata(const char* path, const char* key, char* out_val, size_t max_len) {
    if (!path || !key || !out_val || max_len == 0) return -1;
    out_val[0] = '\0';
    return 0;
}
