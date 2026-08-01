#ifndef BOS_BAR_API_H
#define BOS_BAR_API_H

#include "bar_types.h"

// Master BAR API Declarations
int32_t BAR_Init(void);
int32_t BAR_Shutdown(void);

BARHandle BAR_CreateRuntime(const char* name);
int32_t   BAR_DestroyRuntime(BARHandle runtime);

BARProcessID BAR_CreateProcess(const char* path, const char* app_name);
int32_t      BAR_DestroyProcess(BARProcessID pid);

BARModuleHandle BAR_LoadLibrary(const char* lib_name);
int32_t         BAR_FreeLibrary(BARModuleHandle module);
void*           BAR_GetProcAddress(BARModuleHandle module, const char* symbol_name);

BARWindowID BAR_CreateWindow(BARProcessID pid, int32_t x, int32_t y, int32_t w, int32_t h, const char* title, BARWindowID parent_id);
int32_t     BAR_DestroyWindow(BARWindowID win_id);
int32_t     BAR_ShowWindow(BARWindowID win_id);
int32_t     BAR_HideWindow(BARWindowID win_id);

int32_t     BAR_SetFocus(BARWindowID win_id);
BARWindowID BAR_GetFocus(void);

int32_t BAR_PostMessage(BARWindowID win_id, BARMessageType type, uint32_t p1, uint32_t p2);
int32_t BAR_SendMessage(BARWindowID win_id, BARMessageType type, uint32_t p1, uint32_t p2);
bool    BAR_PeekMessage(BARWindowID win_id, BARMessage* out_msg);
int32_t BAR_DispatchMessage(const BARMessage* msg);

BARTimerID BAR_SetTimer(BARWindowID win_id, uint32_t interval_ms);
int32_t    BAR_KillTimer(BARTimerID timer_id);

BARDialogID BAR_CreateDialog(BARWindowID parent_id, const char* title, int32_t w, int32_t h);
int32_t     BAR_ShowDialog(BARDialogID dlg_id);
int32_t     BAR_CloseDialog(BARDialogID dlg_id);

BARResourceID BAR_LoadResource(BARProcessID pid, const char* res_path);
BARResourceID BAR_LoadIcon(const char* icon_name);
BARResourceID BAR_LoadCursor(const char* cursor_name);

int32_t BAR_OpenClipboard(BARWindowID win_id);
int32_t BAR_CloseClipboard(void);

int32_t BAR_BeginDrag(BARWindowID win_id, const char* mime_type, const void* data, size_t size);
int32_t BAR_EndDrag(void);

int32_t BAR_RegisterPackage(const char* package_path);
int32_t BAR_LoadManifest(const char* manifest_path);

int32_t BAR_ShutdownApplication(BARProcessID pid);

void bar_run_certification_suite(void);

#endif // BOS_BAR_API_H
