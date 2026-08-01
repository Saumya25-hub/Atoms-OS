#include "../include/bfs_api.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"

int32_t BFS_Rename(const char* old_path, const char* new_name) {
    if (!old_path || !new_name) return -1;
    return vfs_rename(old_path, new_name);
}
