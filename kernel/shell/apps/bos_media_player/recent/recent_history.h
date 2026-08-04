#ifndef RECENT_HISTORY_H
#define RECENT_HISTORY_H

#include <stdint.h>
#include <stdbool.h>

#define BOS_RECENT_MAX_ITEMS 8U

typedef struct {
    char items[BOS_RECENT_MAX_ITEMS][128];
    uint32_t count;
} BOS_RecentHistory;

void recent_history_init(BOS_RecentHistory* rh);
void recent_history_push(BOS_RecentHistory* rh, const char* uri);

#endif // RECENT_HISTORY_H
