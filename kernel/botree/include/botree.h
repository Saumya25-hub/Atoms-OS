#ifndef BOTREE_H
#define BOTREE_H

#include "botree_types.h"
#include "botree_path.h"
#include "botree_nav.h"
#include "botree_namespace.h"
#include "botree_cmd.h"
#include "botree_tx.h"

// Directory & Cache Management API
int32_t BDe_ReadDirectory(const char* path, BDeDirEntry** out_entries, uint32_t* out_count);
void    BDe_FreeDirectoryListing(BDeDirEntry* entries);
void    BDe_InvalidateCache(const char* path);

// Directory Watcher API
typedef void (*BDeWatchCallback)(const char* path, uint32_t event_flags, void* user_data);
BDeWatchHandle BDe_WatchSubscribe(const char* path, BDeWatchCallback callback, void* user_data);
void           BDe_WatchUnsubscribe(BDeWatchHandle handle);

// Filesystem Clipboard API
int32_t BDe_ClipboardCopy(const char** paths, uint32_t count);
int32_t BDe_ClipboardCut(const char** paths, uint32_t count);
int32_t BDe_ClipboardPaste(const char* target_dir);

// Permission API
bool BDe_PermissionCheck(const char* path, BDePermRole role, uint32_t required_flags);

// Metadata & Shortcut API
int32_t BDe_ShortcutResolve(const char* link_path, char* out_target, size_t max_len);
int32_t BDe_GetMetadata(const char* path, BDeDirEntry* out_stat);

// BO-TREE Engine Initialization & Shutdown
int32_t BDe_Init(void);
void    BDe_Shutdown(void);

#endif // BOTREE_H
