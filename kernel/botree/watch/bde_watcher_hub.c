#include "../include/botree.h"

BDeWatchHandle BDe_WatchSubscribe(const char* path, BDeWatchCallback callback, void* user_data) {
    (void)path;
    (void)callback;
    (void)user_data;
    static uint32_t s_watch_id = 1;
    return s_watch_id++;
}

void BDe_WatchUnsubscribe(BDeWatchHandle handle) {
    (void)handle;
}
