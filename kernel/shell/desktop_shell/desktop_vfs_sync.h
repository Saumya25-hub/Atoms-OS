#ifndef DESKTOP_VFS_SYNC_H
#define DESKTOP_VFS_SYNC_H

#include <stdint.h>
#include <stdbool.h>

#define DESKTOP_VFS_PATH "/Desktop"
#define DESKTOP_INI_PATH "/Desktop/desktop.ini"

// VFS Synchronization API
void desktop_vfs_sync_init(void);
void desktop_vfs_sync_scan(void);
bool desktop_vfs_save_layout(void);
bool desktop_vfs_load_layout(void);

// File CRUD Helper API
bool desktop_crud_create_folder(const char* folder_name);
bool desktop_crud_create_file(const char* file_name, const char* content);
bool desktop_crud_rename(const char* old_path, const char* new_name);
bool desktop_crud_delete(const char* path, bool permanent);
bool desktop_crud_copy(const char* src_path, const char* dest_dir);
bool desktop_crud_move(const char* src_path, const char* dest_dir);

#endif // DESKTOP_VFS_SYNC_H
