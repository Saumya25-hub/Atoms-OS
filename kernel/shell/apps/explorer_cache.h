#ifndef BOS_EXPLORER_CACHE_H
#define BOS_EXPLORER_CACHE_H

// ============================================================
// Explorer Cache — GUTTED (Phase 9 Rewrite)
// ============================================================
// Legacy ExplorerItem, ExplorerDirCache, ExplorerIconCache
// have been REMOVED. Explorer now uses BSOMObject exclusively.
// This header remains for backward compat of build system.
// ============================================================

#include <stdint.h>
#include <stdbool.h>

void explorer_cache_init(void);
void explorer_cache_invalidate(const char* path);

#endif // BOS_EXPLORER_CACHE_H
