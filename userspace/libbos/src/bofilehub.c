#include "../include/bofilehub.h"
#include "../include/bos.h"

// BO-FileHUB: Folder Engine
int bofh_folder_create(const char* path) {
    return bos_mkdir(path);
}

// BO-FileHUB: File Engine
int bofh_file_create(const char* path) {
    return bos_create(path);
}

// BO-FileHUB: Write Engine
int bofh_file_write(int fd, const void* buffer, size_t size) {
    return bos_write(fd, buffer, size);
}

// BO-FileHUB: Object Management
int bofh_object_rename(const char* old_path, const char* new_name) {
    return bos_rename(old_path, new_name);
}

int bofh_object_delete(const char* path) {
    return bos_delete(path);
}
