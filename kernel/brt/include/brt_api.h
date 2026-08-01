#ifndef BRT_API_H
#define BRT_API_H

#include "brt_types.h"

// Master Subsystem Lifecycle API
int32_t      BRT_Init(void);
BRTRuntime*  BRT_CreateRuntime(uint32_t owner_pid, BRT_AppType app_type);
void         BRT_DestroyRuntime(BRTRuntime* rt);

// Registry API
int32_t      BRT_RegisterRuntime(BRTRuntime* rt, uint32_t window_id);
BRTRuntime*  BRT_GetRuntimeByPID(uint32_t pid);

// Object Manager API
BRTObject*   BRT_CreateObject(BRTRuntime* rt, const char* name, BRTObjectType type);
void         BRT_RetainObject(BRTObject* obj);
void         BRT_ReleaseObject(BRTObject* obj);

// Master Dispatcher API
int32_t      BRT_Open(BRTRuntime* rt, const char* target_path);
int32_t      BRT_Back(BRTRuntime* rt);
int32_t      BRT_Forward(BRTRuntime* rt);
int32_t      BRT_Up(BRTRuntime* rt);
int32_t      BRT_Refresh(BRTRuntime* rt);
int32_t      BRT_Copy(BRTRuntime* rt);
int32_t      BRT_Move(BRTRuntime* rt, const char* dest_dir);
int32_t      BRT_Delete(BRTRuntime* rt, bool send_to_recycle);
int32_t      BRT_Rename(BRTRuntime* rt, const char* old_name, const char* new_name);
int32_t      BRT_NewFolder(BRTRuntime* rt, const char* folder_name);
int32_t      BRT_Launch(const char* file_path);

// Event Hub & Master Clipboard
int32_t      BRT_PostEvent(uint32_t event_id, void* payload);
int32_t      BRT_Subscribe(uint32_t event_id, BRTEventCallback cb);
int32_t      BRT_SetClipboard(const char* path, bool is_cut);
int32_t      BRT_GetClipboard(char* out_path, size_t max_len, bool* out_is_cut);

// Drag & Drop
int32_t      BRT_BeginDrag(BRTRuntime* rt, int32_t start_x, int32_t start_y);
int32_t      BRT_UpdateDrag(BRTRuntime* rt, int32_t cur_x, int32_t cur_y);
int32_t      BRT_EndDrag(BRTRuntime* rt, const char* drop_target);

// Notifications & Search
int32_t      BRT_PushNotification(const char* title, const char* message);
int32_t      BRT_Search(const char* query_pattern, BDeDirEntry** out_results, uint32_t* out_count);

// Transactions
BRTTxHandle  BRT_StartTransaction(BRTRuntime* rt, BRTTxType type);
int32_t      BRT_CommitTransaction(BRTTxHandle tx);
int32_t      BRT_RollbackTransaction(BRTTxHandle tx);

// Session State Save & Restore
int32_t      BRT_SaveRuntime(BRTRuntime* rt);
int32_t      BRT_RestoreRuntime(BRTRuntime* rt);

// Diagnostics Telemetry
void         BRT_GetDiagnostics(BRT_Diagnostics* out_diag);

#endif // BRT_API_H
