#ifndef BSR_API_H
#define BSR_API_H

#include "bsr_types.h"

// Core & Lifecycle
int32_t      BSR_Init(void);
BSR_Runtime* BSR_CreateRuntime(uint32_t owner_pid, BSR_AppType app_type);
void         BSR_DestroyRuntime(BSR_Runtime* rt);

// Dispatcher API
int32_t      BSR_Open(BSR_Runtime* rt, const char* target_path);
int32_t      BSR_Back(BSR_Runtime* rt);
int32_t      BSR_Forward(BSR_Runtime* rt);
int32_t      BSR_Up(BSR_Runtime* rt);
int32_t      BSR_Copy(BSR_Runtime* rt);
int32_t      BSR_Cut(BSR_Runtime* rt);
int32_t      BSR_Paste(BSR_Runtime* rt, const char* target_dir);
int32_t      BSR_Delete(BSR_Runtime* rt, bool send_to_recycle);
int32_t      BSR_Rename(BSR_Runtime* rt, const char* old_name, const char* new_name);
int32_t      BSR_NewFolder(BSR_Runtime* rt, const char* folder_name);

// Launcher & Associations
int32_t      BSR_Launch(const char* file_path);
int32_t      BSR_RegisterAssociation(const char* ext, const char* app_name, const char* app_path);
const char*  BSR_GetAssociation(const char* ext);

// File Dialog & Search
int32_t      BSR_ShowFileDialog(BSR_Runtime* rt, const char* title, bool is_save, char* out_selected_path, size_t max_len);
int32_t      BSR_Search(const char* query_pattern, BDeDirEntry** out_results, uint32_t* out_count);

// Recent & Favorites
int32_t      BSR_AddRecent(const char* path);
int32_t      BSR_AddFavorite(const char* name, const char* path);

// Notifications & Diagnostics
int32_t      BSR_PushNotification(const char* title, const char* message);
void         BSR_GetDiagnostics(BSR_Diagnostics* out_diag);

#endif // BSR_API_H
