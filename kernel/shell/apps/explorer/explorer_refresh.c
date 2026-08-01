#include "explorer_refresh.h"
#include "kernel/shell/apps/explorer_cache.h"

void explorer_refresh_init(ExplorerRefreshEngine* re) {
    if (!re) return;
    re->refresh_count = 0;
    re->last_reason = REFRESH_REASON_MANUAL;
    re->auto_refresh_enabled = true;
}

void explorer_refresh_trigger(ExplorerRefreshEngine* re, RefreshReason reason, const char* path) {
    if (!re) return;
    re->refresh_count++;
    re->last_reason = reason;
    if (path) {
        explorer_cache_invalidate(path);
    }
}
