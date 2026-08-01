#include "../include/bsr_api.h"
#include "kernel/core/lib/include/string.h"

static char     s_recents[BSR_MAX_RECENTS][BDE_PATH_MAX];
static uint32_t s_recent_count = 0;

int32_t BSR_AddRecent(const char* path) {
    if (!path || strlen(path) == 0) return -1;

    for (uint32_t i = 0; i < s_recent_count; i++) {
        if (strcmp(s_recents[i], path) == 0) return 0;
    }

    if (s_recent_count < BSR_MAX_RECENTS) {
        strcpy(s_recents[s_recent_count++], path);
    } else {
        for (int i = 0; i < BSR_MAX_RECENTS - 1; i++) {
            strcpy(s_recents[i], s_recents[i + 1]);
        }
        strcpy(s_recents[BSR_MAX_RECENTS - 1], path);
    }
    return 0;
}
