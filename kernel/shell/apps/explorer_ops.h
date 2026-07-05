#ifndef BOS_EXPLORER_OPS_H
#define BOS_EXPLORER_OPS_H

#include "explorer.h"

// Basic stubs for future file operations
void explorer_ops_copy(const char* src_path);
void explorer_ops_paste(const char* dest_path);
void explorer_ops_delete(const char* path);
void explorer_ops_rename(const char* old_path, const char* new_path);
void explorer_ops_new_folder(const char* path, const char* name);

#endif // BOS_EXPLORER_OPS_H
