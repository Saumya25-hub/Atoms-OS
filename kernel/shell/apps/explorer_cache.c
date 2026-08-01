// ============================================================
// Explorer Cache — GUTTED (Phase 9 Rewrite)
// ============================================================
// ALL legacy VFS code removed. Explorer cache now delegates
// entirely to BSOM object cache. This file is maintained for
// backward compat only — all logic lives in BSOM.
// ============================================================

#include "explorer_cache.h"
#include "kernel/core/lib/include/string.h"

void explorer_cache_init(void) {
    // BSOM handles all caching. Nothing to do here.
}

void explorer_cache_invalidate(const char* path) {
    (void)path;
    // BSOM handles cache invalidation.
}
