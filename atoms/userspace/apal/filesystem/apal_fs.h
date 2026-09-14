/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Filesystem Adapter (Chromium base::File / file_util)
 */

#ifndef ATOMS_APAL_FS_H
#define ATOMS_APAL_FS_H

#include "../include/apal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int apal_file_handle_t;
#define APAL_INVALID_FILE_HANDLE (-1)

typedef enum {
    APAL_FILE_OPEN_READ         = 0x01,
    APAL_FILE_OPEN_WRITE        = 0x02,
    APAL_FILE_OPEN_CREATE       = 0x04,
    APAL_FILE_OPEN_TRUNCATE     = 0x08,
    APAL_FILE_OPEN_APPEND       = 0x10
} apal_file_flags_t;

typedef enum {
    APAL_SEEK_SET = 0,
    APAL_SEEK_CUR = 1,
    APAL_SEEK_END = 2
} apal_seek_origin_t;

typedef struct {
    uint64_t size_bytes;
    uint64_t creation_time;
    uint64_t last_modified_time;
    bool is_directory;
} apal_file_info_t;

apal_status_t apal_file_open(const char *path, uint32_t flags, apal_file_handle_t *out_handle);
apal_status_t apal_file_close(apal_file_handle_t handle);
int64_t apal_file_read(apal_file_handle_t handle, void *buffer, size_t bytes_to_read);
int64_t apal_file_write(apal_file_handle_t handle, const void *buffer, size_t bytes_to_write);
int64_t apal_file_seek(apal_file_handle_t handle, int64_t offset, apal_seek_origin_t origin);
apal_status_t apal_file_stat(const char *path, apal_file_info_t *out_info);
apal_status_t apal_file_fstat(apal_file_handle_t handle, apal_file_info_t *out_info);
apal_status_t apal_file_create_dir(const char *path);
apal_status_t apal_file_delete(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_FS_H */
