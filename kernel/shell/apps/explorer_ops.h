#ifndef BOS_EXPLORER_OPS_H
#define BOS_EXPLORER_OPS_H

#include "explorer.h"

void explorer_ops_copy(ExplorerContext* ctx, const char* src_path);
void explorer_ops_paste(ExplorerContext* ctx, const char* dest_path);
void explorer_ops_delete(ExplorerContext* ctx, const char* path);
void explorer_ops_rename(ExplorerContext* ctx, const char* old_path, const char* new_path);
void explorer_ops_new_folder(ExplorerContext* ctx, const char* path, const char* name);

#endif // BOS_EXPLORER_OPS_H
