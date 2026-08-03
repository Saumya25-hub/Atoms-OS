#ifndef RENAME_DIALOG_H
#define RENAME_DIALOG_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*RenameCallback)(const char* old_path, const char* new_name, void* user_data);

void RenameDialog_Open(const char* target_path, const char* initial_name, RenameCallback cb, void* user_data);

#endif // RENAME_DIALOG_H
