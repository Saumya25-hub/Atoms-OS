#ifndef BFS_API_H
#define BFS_API_H

#include "bfs_types.h"

// Subsystem Init
int32_t         BFS_Init(void);

// 1. File Manager & I/O
BFS_FileHandle  BFS_Open(const char* path, uint32_t mode);
void            BFS_Close(BFS_FileHandle handle);
int32_t         BFS_Read(BFS_FileHandle handle, void* buf, size_t count);
int32_t         BFS_Write(BFS_FileHandle handle, const void* buf, size_t count);
bool            BFS_Exists(const char* path);
int32_t         BFS_Stat(const char* path, BFS_StatStruct* out_stat);

// 2-6. Core Operations (Copy, Move, Delete, Rename, Create)
BFSTxHandle     BFS_Copy(const char* src, const char* dest, uint32_t flags);
BFSTxHandle     BFS_Move(const char* src, const char* dest, uint32_t flags);
BFSTxHandle     BFS_Delete(const char* path, bool send_to_recycle);
int32_t         BFS_Rename(const char* old_path, const char* new_name);
int32_t         BFS_CreateFile(const char* path);
int32_t         BFS_CreateFolder(const char* path);

// 7-12. Properties, Locks, Associations, MIME, Icons, Thumbnails
int32_t         BFS_GetProperties(const char* path, BFS_Properties* out_props);
const char*     BFS_GetMime(const char* path);
uint32_t        BFS_GetIcon(const char* path);
void*           BFS_GetThumbnail(const char* path, uint32_t width, uint32_t height);
const char*     BFS_GetAssociation(const char* extension);
int32_t         BFS_Lock(const char* path, uint32_t lock_type);
int32_t         BFS_Unlock(const char* path);

// 13-16. Recycle Bin, Recent Files, Favorites, Quick Access
int32_t         BFS_MoveToRecycle(const char* path);
int32_t         BFS_Restore(const char* recycle_id);
int32_t         BFS_GetRecent(BFS_ItemEntry* out_items, uint32_t* out_count);
int32_t         BFS_PinFavorite(const char* path);
int32_t         BFS_UnpinFavorite(const char* path);
int32_t         BFS_GetQuickAccess(BFS_ItemEntry* out_items, uint32_t* out_count);

// 18-20. Search, Transactions & Diagnostics
int32_t         BFS_Search(const char* pattern, BFS_ItemEntry** out_results, uint32_t* out_count);
BFSTxHandle     BFS_BeginTransaction(BFSTxType type);
int32_t         BFS_CommitTransaction(BFSTxHandle tx);
int32_t         BFS_RollbackTransaction(BFSTxHandle tx);
void            BFS_GetDiagnostics(BFS_Diagnostics* out_diag);

#endif // BFS_API_H
