#ifndef DRE_API_H
#define DRE_API_H

#include "dre_types.h"

// Public Directory Runtime API (BDeRuntime_*)
BDeRuntime* BDeRuntime_Create(uint32_t owner_pid, BDeViewMode initial_view_mode);
void        BDeRuntime_Destroy(BDeRuntime* rt);

// Navigation API
int32_t     BDeRuntime_Open(BDeRuntime* rt, const char* path);
int32_t     BDeRuntime_Back(BDeRuntime* rt);
int32_t     BDeRuntime_Forward(BDeRuntime* rt);
int32_t     BDeRuntime_Up(BDeRuntime* rt);
int32_t     BDeRuntime_Refresh(BDeRuntime* rt);
const char* BDeRuntime_GetCurrentDirectory(BDeRuntime* rt);

// Enumeration & Query API
int32_t     BDeRuntime_Enumerate(BDeRuntime* rt, BDeDirEntry** out_entries, uint32_t* out_count);
int32_t     BDeRuntime_GetBreadcrumb(BDeRuntime* rt, BDeBreadcrumbSegment* out_segments, uint32_t* out_count);

// Selection API
int32_t     BDeRuntime_Select(BDeRuntime* rt, int32_t index);
int32_t     BDeRuntime_DeselectAll(BDeRuntime* rt);
int32_t     BDeRuntime_SelectAll(BDeRuntime* rt);
uint32_t    BDeRuntime_GetSelectedCount(BDeRuntime* rt);

// Sorting & Filtering API
int32_t     BDeRuntime_SetSort(BDeRuntime* rt, BDeSortField field, bool ascending);
int32_t     BDeRuntime_SetFilter(BDeRuntime* rt, const char* extension_pattern, bool show_hidden);

// File Operations via Transaction Engine
BDeTxHandle BDeRuntime_CopySelected(BDeRuntime* rt, const char* target_dir);
BDeTxHandle BDeRuntime_MoveSelected(BDeRuntime* rt, const char* target_dir);
BDeTxHandle BDeRuntime_DeleteSelected(BDeRuntime* rt, bool send_to_recycle);

// Diagnostics Telemetry
void        BDeRuntime_GetDiagnostics(BDeRuntimeDiagnostics* out_diag);

#endif // DRE_API_H
