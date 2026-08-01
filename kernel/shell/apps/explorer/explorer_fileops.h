#ifndef BSEC_EXPLORER_FILEOPS_H
#define BSEC_EXPLORER_FILEOPS_H

#include <stdint.h>
#include <stdbool.h>
#include "explorer_clipboard.h"

int explorer_fileops_new_folder(const char* target_dir, const char* name);
int explorer_fileops_rename(const char* old_path, const char* new_path);
int explorer_fileops_delete(const char* path);
int explorer_fileops_copy_file(const char* src_path, const char* dest_dir);
int explorer_fileops_move_file(const char* src_path, const char* dest_dir);
int explorer_fileops_paste_clipboard(const ExplorerClipboard* cb, const char* dest_dir);

#endif // BSEC_EXPLORER_FILEOPS_H
