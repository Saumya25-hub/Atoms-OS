#include "../include/bfs_api.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"

int32_t BFS_CreateFile(const char* path) {
    if (!path) return -1;
    return vfs_create(path);
}

int32_t BFS_CreateFolder(const char* path) {
    if (!path) return -1;
    return vfs_mkdir(path);
}
