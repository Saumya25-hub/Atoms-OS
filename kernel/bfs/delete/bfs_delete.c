#include "../include/bfs_api.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"

BFSTxHandle BFS_Delete(const char* path, bool send_to_recycle) {
    if (!path) return 0;
    if (send_to_recycle) {
        BFS_MoveToRecycle(path);
    } else {
        vfs_delete(path);
    }
    return BFS_BeginTransaction(BFS_TX_DELETE);
}
