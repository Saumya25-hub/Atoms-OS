#include "../include/bfs_api.h"

int32_t BFS_Lock(const char* path, uint32_t lock_type) {
    if (!path) return -1;
    (void)lock_type;
    return 0;
}

int32_t BFS_Unlock(const char* path) {
    if (!path) return -1;
    return 0;
}
