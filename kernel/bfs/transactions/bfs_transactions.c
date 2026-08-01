#include "../include/bfs_api.h"

static uint32_t s_next_bfs_tx = 5000;

BFSTxHandle BFS_BeginTransaction(BFSTxType type) {
    (void)type;
    return s_next_bfs_tx++;
}

int32_t BFS_CommitTransaction(BFSTxHandle tx) {
    if (tx == 0) return -1;
    return 0;
}

int32_t BFS_RollbackTransaction(BFSTxHandle tx) {
    if (tx == 0) return -1;
    return 0;
}
