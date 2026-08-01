#ifndef BOS_FILEEXPLORER_API_H
#define BOS_FILEEXPLORER_API_H

#include "fileexplorer_types.h"

// Lifecycle
int32_t  FileExplorerInitialize(void);
void     FileExplorerShutdown(void);

// Navigation
bool     fe_navigate_to(const char* path);
bool     fe_navigate_back(void);
bool     fe_navigate_forward(void);
bool     fe_navigate_up(void);
const char* fe_get_current_path(void);

// Address Bar
bool     fe_addressbar_set(const char* path);
const char* fe_addressbar_get(void);

// Listing
bool     fe_refresh_listing(void);
uint32_t fe_entry_count(void);
FE_ENTRY* fe_entry_get(uint32_t idx);

// Drive Manager
bool     fe_refresh_drives(void);
uint32_t fe_drive_count(void);
FE_DRIVE* fe_drive_get(uint32_t idx);

// File Operations
uint32_t fe_op_copy(const char* src, const char* dst);
uint32_t fe_op_move(const char* src, const char* dst);
bool     fe_op_delete(const char* path);
bool     fe_op_rename(const char* path, const char* new_name);
bool     fe_op_create_dir(const char* path);
bool     fe_op_create_file(const char* path);
bool     fe_op_pause(uint32_t op_id);
bool     fe_op_resume(uint32_t op_id);
bool     fe_op_cancel(uint32_t op_id);

// Search
bool     fe_search_start(const FE_SEARCH_QUERY* query);
uint32_t fe_search_result_count(void);
FE_ENTRY* fe_search_result_get(uint32_t idx);

// Clipboard
bool     fe_clipboard_copy(const char* path);
bool     fe_clipboard_cut(const char* path);
bool     fe_clipboard_paste(const char* dst_dir);

// DragDrop
bool     fe_dragdrop_begin(const char* path);
bool     fe_dragdrop_drop(const char* dst_dir);

// Favorites
bool     fe_favorites_add(const char* path);
bool     fe_favorites_remove(const char* path);
uint32_t fe_favorites_count(void);

// Recent
uint32_t fe_recent_count(void);
const char* fe_recent_get(uint32_t idx);

// History
uint32_t fe_history_count(void);
FE_HISTORY_ENTRY* fe_history_get(uint32_t idx);

// Properties
bool     fe_properties_show(const char* path);
bool     fe_permissions_show(const char* path);

// Thumbnails
bool     fe_thumbnail_request(const char* path);
bool     fe_thumbnail_invalidate(const char* path);

// Watcher
bool     fe_watcher_start(const char* path);
bool     fe_watcher_stop(const char* path);

// Diagnostics
FE_DIAGNOSTICS* fe_get_diagnostics(void);
void     FileExplorerDumpDiagnostics(void);

// Forensic Mode
bool     fe_forensic_open(const char* path, FE_ENTRY* out);

#endif // BOS_FILEEXPLORER_API_H
