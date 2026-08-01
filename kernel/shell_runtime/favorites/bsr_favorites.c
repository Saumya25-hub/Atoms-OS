#include "../include/bsr_api.h"
#include "kernel/core/lib/include/string.h"

static struct {
    char name[64];
    char path[BDE_PATH_MAX];
} s_favorites[BSR_MAX_FAVORITES];
static uint32_t s_favorite_count = 0;

int32_t BSR_AddFavorite(const char* name, const char* path) {
    if (!name || !path) return -1;
    if (s_favorite_count < BSR_MAX_FAVORITES) {
        strcpy(s_favorites[s_favorite_count].name, name);
        strcpy(s_favorites[s_favorite_count].path, path);
        s_favorite_count++;
        return 0;
    }
    return -1;
}
