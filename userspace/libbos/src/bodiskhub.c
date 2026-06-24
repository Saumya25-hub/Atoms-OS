#include "../include/bodiskhub.h"
#include "../include/bos.h"

// BO-DiskHUB: Folder Engine
int bodh_folder_create(const char* path) {
    return bos_mkdir(path);
}

// BO-DiskHUB: File Engine
int bodh_file_create(const char* path) {
    return bos_create(path);
}

// BO-DiskHUB: Write Engine
int bodh_file_write(int fd, const void* buffer, size_t size) {
    return bos_write(fd, buffer, size);
}

// BO-DiskHUB: Object Management
int bodh_object_rename(const char* old_path, const char* new_name) {
    return bos_rename(old_path, new_name);
}

int bodh_object_delete(const char* path) {
    return bos_delete(path);
}
