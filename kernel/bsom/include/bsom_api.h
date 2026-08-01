#ifndef BSOM_API_H
#define BSOM_API_H

#include "bsom_types.h"

// Subsystem Init
int32_t       BSOM_Init(void);

// Object Creation, Opening & Refcounting
BSOMObject*   BSOM_CreateObject(const char* name, BSOMClassType class_type);
BSOMHandle    BSOM_OpenObject(const char* path_or_uri);
void          BSOM_CloseObject(BSOMHandle handle);
void          BSOM_Retain(BSOMObject* obj);
void          BSOM_Release(BSOMObject* obj);

// Properties
int32_t       BSOM_GetProperty(BSOMObject* obj, const char* key, char* out_val, size_t max_len);
int32_t       BSOM_SetProperty(BSOMObject* obj, const char* key, const char* val);
int32_t       BSOM_ShowProperties(BSOMObject* obj);

// Hierarchy & Operations
int32_t       BSOM_GetChildren(BSOMObject* obj, BSOMObject*** out_children, uint32_t* out_count);
int32_t       BSOM_Enumerate(BSOMObject* obj, BSOMEnumCallback cb);
int32_t       BSOM_Copy(BSOMObject* obj, BSOMObject* dest_folder);
int32_t       BSOM_Move(BSOMObject* obj, BSOMObject* dest_folder);
int32_t       BSOM_Delete(BSOMObject* obj, bool send_to_recycle);
int32_t       BSOM_Rename(BSOMObject* obj, const char* new_name);

// Shortcuts & App Execution
BSOMObject*   BSOM_CreateShortcut(const char* target_path, const char* shortcut_path);
int32_t       BSOM_ResolveShortcut(BSOMObject* shortcut_obj, char* out_target_path);
int32_t       BSOM_Invoke(BSOMObject* obj);

// Context Menu, Icons & Thumbnails
int32_t       BSOM_GetContextMenu(BSOMObject* obj, BSOMContextMenu* out_menu);
uint32_t      BSOM_GetIcon(BSOMObject* obj);
void*         BSOM_GetThumbnail(BSOMObject* obj, uint32_t w, uint32_t h);

// Favorites, Quick Access & Recent Files
int32_t       BSOM_AddFavorite(BSOMObject* obj);
int32_t       BSOM_RemoveFavorite(BSOMObject* obj);
int32_t       BSOM_PinQuickAccess(BSOMObject* obj);
int32_t       BSOM_UnpinQuickAccess(BSOMObject* obj);
int32_t       BSOM_GetRecent(BSOMObject*** out_items, uint32_t* out_count);

// Search & Locking
int32_t       BSOM_Search(const char* pattern, BSOMObject*** out_results, uint32_t* out_count);
int32_t       BSOM_Lock(BSOMObject* obj, uint32_t lock_type);
int32_t       BSOM_Unlock(BSOMObject* obj);

// Diagnostics
void          BSOM_GetDiagnostics(BSOM_Diagnostics* out_diag);

#endif // BSOM_API_H
