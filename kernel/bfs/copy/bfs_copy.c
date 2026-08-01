#include "../include/bfs_api.h"

BFSTxHandle BFS_Copy(const char* src, const char* dest, uint32_t flags) {
    if (!src || !dest) return 0;
    (void)flags;
    return BFS_BeginTransaction(BFS_TX_COPY);
}
