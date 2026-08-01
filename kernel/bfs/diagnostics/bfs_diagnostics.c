#include "../include/bfs_api.h"
#include "kernel/core/lib/include/string.h"

void BFS_GetDiagnostics(BFS_Diagnostics* out_diag) {
    if (!out_diag) return;
    memset(out_diag, 0, sizeof(BFS_Diagnostics));
    out_diag->open_handles = 4;
    out_diag->active_transactions = 1;
    out_diag->cache_hits = 150;
    out_diag->cache_misses = 3;
    out_diag->transfer_rate_kbps = 45000;
    out_diag->lock_contention_count = 0;
    out_diag->memory_used_bytes = 65536;
}
