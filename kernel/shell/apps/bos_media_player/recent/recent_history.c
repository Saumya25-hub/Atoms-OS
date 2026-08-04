#include "recent_history.h"
#include "kernel/core/lib/include/string.h"

void recent_history_init(BOS_RecentHistory* rh) {
    if (!rh) return;
    memset(rh, 0, sizeof(BOS_RecentHistory));
}

void recent_history_push(BOS_RecentHistory* rh, const char* uri) {
    if (!rh || !uri) return;

    if (rh->count < BOS_RECENT_MAX_ITEMS) {
        strncpy(rh->items[rh->count], uri, sizeof(rh->items[rh->count]) - 1);
        rh->count++;
    } else {
        for (uint32_t i = 0; i < BOS_RECENT_MAX_ITEMS - 1; i++) {
            strncpy(rh->items[i], rh->items[i + 1], sizeof(rh->items[i]) - 1);
        }
        strncpy(rh->items[BOS_RECENT_MAX_ITEMS - 1], uri, sizeof(rh->items[0]) - 1);
    }
}
