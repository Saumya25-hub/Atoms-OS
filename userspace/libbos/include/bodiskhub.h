#ifndef BO_DISKHUB_H
#define BO_DISKHUB_H

#include <stdint.h>
#include <stddef.h>

// BO-DiskHUB Object Types
typedef enum {
    BO_OBJECT_FILE,
    BO_OBJECT_FOLDER,
    BO_OBJECT_SHORTCUT,
    BO_OBJECT_APP
} BOObjectType;

// BO-DiskHUB Object Structure
typedef struct {
    BOObjectType type;
    char name[64];
    uint64_t size;
} BOObject;

// Core Engines API
int bodh_folder_create(const char* path);
int bodh_file_create(const char* path);
int bodh_file_write(int fd, const void* buffer, size_t size);
int bodh_object_rename(const char* old_path, const char* new_name);
int bodh_object_delete(const char* path);

#endif // BO_DISKHUB_H
