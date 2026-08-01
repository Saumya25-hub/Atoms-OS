#include "../include/bfs_api.h"
#include "kernel/core/lib/include/string.h"

static char     s_favorites[BFS_MAX_FAVORITES][BDE_PATH_MAX];
static uint32_t s_fav_count = 0;

int32_t BFS_PinFavorite(const char* path) {
    if (!path) return -1;
    if (s_fav_count < BFS_MAX_FAVORITES) {
        strcpy(s_favorites[s_fav_count++], path);
        return 0;
    }
    return -1;
}

int32_t BFS_UnpinFavorite(const char* path) {
    if (!path) return -1;
    return 0;
}
