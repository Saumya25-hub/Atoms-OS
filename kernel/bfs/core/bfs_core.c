#include "../include/bfs_api.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

static bool s_bfs_inited = false;

int32_t BFS_Init(void) {
    if (s_bfs_inited) return 0;
    display_print("[BFS] Initializing BO-TREE Filesystem Services V1.0...\n");
    s_bfs_inited = true;
    return 0;
}
