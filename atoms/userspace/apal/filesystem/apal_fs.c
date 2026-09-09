/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Filesystem Implementation
 */

#include "apal_fs.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"
#include <string.h>

#define O_RDONLY 00
#define O_WRONLY 01
#define O_RDWR   02
#define O_CREAT  0100
#define O_TRUNC  01000
#define O_APPEND 02000

apal_status_t apal_file_open(const char *path, uint32_t flags, apal_file_handle_t *out_handle) {
    if (!path || !out_handle) return APAL_ERR_INVALID_PARAM;

    int posix_flags = 0;
    if ((flags & APAL_FILE_OPEN_READ) && (flags & APAL_FILE_OPEN_WRITE)) {
        posix_flags |= O_RDWR;
    } else if (flags & APAL_FILE_OPEN_WRITE) {
        posix_flags |= O_WRONLY;
    } else {
        posix_flags |= O_RDONLY;
    }

    if (flags & APAL_FILE_OPEN_CREATE) posix_flags |= O_CREAT;
    if (flags & APAL_FILE_OPEN_TRUNCATE) posix_flags |= O_TRUNC;
    if (flags & APAL_FILE_OPEN_APPEND) posix_flags |= O_APPEND;

    int64_t fd = __syscall3(SYS_OPEN, (int64_t)path, (int64_t)posix_flags, 0644);
    if (fd < 0) {
        *out_handle = APAL_INVALID_FILE_HANDLE;
        return APAL_ERR_NOT_FOUND;
    }

    *out_handle = (apal_file_handle_t)fd;
    return APAL_OK;
}

apal_status_t apal_file_close(apal_file_handle_t handle) {
    if (handle < 0) return APAL_ERR_INVALID_PARAM;
    int64_t ret = __syscall1(SYS_CLOSE, (int64_t)handle);
    return (ret == 0) ? APAL_OK : APAL_ERR_IO;
}

int64_t apal_file_read(apal_file_handle_t handle, void *buffer, size_t bytes_to_read) {
    if (handle < 0 || !buffer) return -1;
    return __syscall3(SYS_READ, (int64_t)handle, (int64_t)buffer, (int64_t)bytes_to_read);
}

int64_t apal_file_write(apal_file_handle_t handle, const void *buffer, size_t bytes_to_write) {
    if (handle < 0 || !buffer) return -1;
    return __syscall3(SYS_WRITE_FILE, (int64_t)handle, (int64_t)buffer, (int64_t)bytes_to_write);
}

int64_t apal_file_seek(apal_file_handle_t handle, int64_t offset, apal_seek_origin_t origin) {
    if (handle < 0) return -1;
    int whence = 0;
    if (origin == APAL_SEEK_SET) whence = 0;
    else if (origin == APAL_SEEK_CUR) whence = 1;
    else if (origin == APAL_SEEK_END) whence = 2;

    return __syscall3(SYS_SEEK, (int64_t)handle, offset, (int64_t)whence);
}

apal_status_t apal_file_stat(const char *path, apal_file_info_t *out_info) {
    if (!path || !out_info) return APAL_ERR_INVALID_PARAM;
    atoms_stat_t st;
    memset(&st, 0, sizeof(st));

    int64_t ret = __syscall2(SYS_STAT, (int64_t)path, (int64_t)&st);
    if (ret != 0) return APAL_ERR_NOT_FOUND;

    out_info->size_bytes = st.st_size;
    out_info->creation_time = st.st_ctime;
    out_info->last_modified_time = st.st_mtime;
    out_info->is_directory = ((st.st_mode & 0040000) != 0);
    return APAL_OK;
}

apal_status_t apal_file_fstat(apal_file_handle_t handle, apal_file_info_t *out_info) {
    if (handle < 0 || !out_info) return APAL_ERR_INVALID_PARAM;
    /* In ATOMS, query stat via seek or direct table */
    int64_t cur = apal_file_seek(handle, 0, APAL_SEEK_CUR);
    int64_t end = apal_file_seek(handle, 0, APAL_SEEK_END);
    apal_file_seek(handle, cur, APAL_SEEK_SET);

    out_info->size_bytes = (end >= 0) ? (uint64_t)end : 0;
    out_info->creation_time = 0;
    out_info->last_modified_time = 0;
    out_info->is_directory = false;
    return APAL_OK;
}

apal_status_t apal_file_create_dir(const char *path) {
    if (!path) return APAL_ERR_INVALID_PARAM;
    int64_t ret = __syscall2(SYS_MKDIR, (int64_t)path, 0755);
    return (ret == 0) ? APAL_OK : APAL_ERR_IO;
}

apal_status_t apal_file_delete(const char *path) {
    if (!path) return APAL_ERR_INVALID_PARAM;
    int64_t ret = __syscall1(SYS_UNLINK, (int64_t)path);
    return (ret == 0) ? APAL_OK : APAL_ERR_IO;
}
