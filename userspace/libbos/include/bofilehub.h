#ifndef BO_FILEHUB_H
#define BO_FILEHUB_H

#include <stdint.h>
#include <stddef.h>

// BO-FileHUB Object Types
typedef enum {
    BO_OBJECT_FILE,
    BO_OBJECT_FOLDER,
    BO_OBJECT_SHORTCUT,
    BO_OBJECT_APP
} BOObjectType;

// BO-FileHUB Object Structure
typedef struct {
    BOObjectType type;
    char name[64];
    uint64_t size;
} BOObject;

// Core Engines API
int bofh_folder_create(const char* path);
int bofh_file_create(const char* path);
int bofh_file_write(int fd, const void* buffer, size_t size);
int bofh_object_rename(const char* old_path, const char* new_name);
int bofh_object_delete(const char* path);

#endif // BO_FILEHUB_H
