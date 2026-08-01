#include "explorer_fileops.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/lib/include/string.h"

static void exp_concat_path(char* out, const char* dir, const char* name) {
    strcpy(out, dir);
    int len = strlen(out);
    if (len > 0 && out[len - 1] != '/') {
        out[len] = '/';
        out[len + 1] = '\0';
    }
    strcat(out, name);
}

static const char* exp_get_basename(const char* path) {
    if (!path) return "";
    int len = strlen(path);
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') return &path[i + 1];
    }
    return path;
}

int explorer_fileops_new_folder(const char* target_dir, const char* name) {
    if (!target_dir || !name) return -1;
    char full_path[256];
    exp_concat_path(full_path, target_dir, name);
    return vfs_mkdir(full_path);
}

int explorer_fileops_rename(const char* old_path, const char* new_path) {
    if (!old_path || !new_path) return -1;
    return vfs_rename(old_path, new_path);
}

int explorer_fileops_delete(const char* path) {
    if (!path) return -1;
    return vfs_delete(path);
}

int explorer_fileops_copy_file(const char* src_path, const char* dest_dir) {
    if (!src_path || !dest_dir) return -1;
    
    int src_fd = vfs_open(src_path);
    if (src_fd < 0) return -1;
    
    const char* filename = exp_get_basename(src_path);
    char dest_path[256];
    exp_concat_path(dest_path, dest_dir, filename);
    
    int dest_fd = vfs_create(dest_path);
    if (dest_fd < 0) {
        vfs_close(src_fd);
        return -1;
    }
    
    static uint8_t buffer[4096];
    int read_bytes = 0;
    while ((read_bytes = vfs_read(src_fd, buffer, sizeof(buffer))) > 0) {
        vfs_write(dest_fd, buffer, read_bytes);
    }
    
    vfs_close(src_fd);
    vfs_close(dest_fd);
    return 0;
}

int explorer_fileops_move_file(const char* src_path, const char* dest_dir) {
    if (!src_path || !dest_dir) return -1;
    const char* filename = exp_get_basename(src_path);
    char dest_path[256];
    exp_concat_path(dest_path, dest_dir, filename);
    
    // First try atomic rename
    if (vfs_rename(src_path, dest_path) == 0) return 0;
    
    // Fall back to copy + delete (cross-filesystem or cross-volume)
    if (explorer_fileops_copy_file(src_path, dest_dir) == 0) {
        return vfs_delete(src_path);
    }
    return -1;
}

int explorer_fileops_paste_clipboard(const ExplorerClipboard* cb, const char* dest_dir) {
    if (!cb || !dest_dir || cb->count == 0) return -1;
    int success_count = 0;
    for (uint32_t i = 0; i < cb->count; i++) {
        if (cb->op == CLIPBOARD_OP_COPY) {
            if (explorer_fileops_copy_file(cb->paths[i], dest_dir) == 0) success_count++;
        } else if (cb->op == CLIPBOARD_OP_CUT) {
            if (explorer_fileops_move_file(cb->paths[i], dest_dir) == 0) success_count++;
        }
    }
    return (success_count == (int)cb->count) ? 0 : -1;
}
