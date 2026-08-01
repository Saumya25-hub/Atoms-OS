#include "../include/bfs_api.h"
#include "kernel/core/lib/include/string.h"

int32_t BFS_GetProperties(const char* path, BFS_Properties* out_props) {
    if (!path || !out_props) return -1;
    memset(out_props, 0, sizeof(BFS_Properties));
    strcpy(out_props->path, path);
    strcpy(out_props->mime_type, BFS_GetMime(path));
    out_props->icon_id = BFS_GetIcon(path);
    out_props->size_bytes = 4096;
    return 0;
}
