#include "bospectra_file.h"
#include "../include/bospectra_errors.h"
#include "../debug/bospectra_debug.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_OPEN_FILES 8U

static BOSPECTRA_FileContext g_bospectra_files[BOSPECTRA_MAX_OPEN_FILES];
static bool g_bospectra_file_initialized = false;

void bospectra_file_subsystem_init(void) {
    memset(g_bospectra_files, 0, sizeof(g_bospectra_files));
    g_bospectra_file_initialized = true;
}

void bospectra_file_subsystem_shutdown(void) {
    for (uint32_t i = 0; i < BOSPECTRA_MAX_OPEN_FILES; i++) {
        if (g_bospectra_files[i].is_open) {
            bospectra_file_close(g_bospectra_files[i].id);
        }
    }
    memset(g_bospectra_files, 0, sizeof(g_bospectra_files));
    g_bospectra_file_initialized = false;
}

bospectra_error_t bospectra_file_open(const char* path, bospectra_file_id_t* out_file_id) {
    if (!g_bospectra_file_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!path || !out_file_id) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    bospectra_trace_str("TRACE 3 — File Open Requested Path", path);

    /* Helper: extract just the filename after the last '/' */
    const char* basename = path;
    for (const char* p = path; *p; p++) {
        if (*p == '/') basename = p + 1;
    }

    /* Uppercase the basename for FAT32 8.3 format matching */
    char upper_name[32];
    uint32_t ui = 0;
    for (; basename[ui] && ui < 31; ui++) {
        char c = basename[ui];
        upper_name[ui] = (c >= 'a' && c <= 'z') ? (c - 32) : c;
    }
    upper_name[ui] = '\0';

    /* Try path variations in order until one succeeds */
    const char* candidates[8];
    char no_slash[128];
    char media_stripped[128];
    uint32_t k = 0;

    candidates[k++] = path;                       /* Original as-is */
    if (path[0] == '/') {
        strncpy(no_slash, path + 1, 127);
        candidates[k++] = no_slash;               /* Strip leading slash */
    }
    if (strncmp(path, "/Media/", 7) == 0) {
        strncpy(media_stripped, path + 7, 127);
        candidates[k++] = media_stripped;         /* Strip /Media/ prefix */
    }
    candidates[k++] = basename;                   /* Just the filename */
    candidates[k++] = upper_name;                 /* UPPERCASE.EXT for FAT32 */

    const char* resolved_path = path;
    int fd = -1;
    for (uint32_t ci = 0; ci < k && fd < 0; ci++) {
        if (candidates[ci] && candidates[ci][0]) {
            fd = vfs_open(candidates[ci]);
            if (fd >= 0) resolved_path = candidates[ci];
        }
    }

    bospectra_trace_str("Resolved Path", resolved_path);
    bospectra_trace_u32("File Handle (FD)", (uint32_t)fd);

    if (fd < 0) {
        bospectra_trace_str("File Open Result", "FAILED (BOSPECTRA_ERR_FILE_NOT_FOUND)");
        return BOSPECTRA_ERR_FILE_NOT_FOUND;
    }

    uint64_t real_size = (uint64_t)vfs_seek(fd, 0, 2); /* SEEK_END */
    vfs_seek(fd, 0, 0); /* SEEK_SET back to beginning */

    for (uint32_t i = 0; i < BOSPECTRA_MAX_OPEN_FILES; i++) {
        if (!g_bospectra_files[i].is_open) {
            g_bospectra_files[i].id = i + 1;
            g_bospectra_files[i].vfs_fd = fd;
            strncpy(g_bospectra_files[i].path, path, sizeof(g_bospectra_files[i].path) - 1);
            g_bospectra_files[i].current_offset = 0;
            g_bospectra_files[i].file_size = real_size;
            g_bospectra_files[i].is_open = true;

            *out_file_id = g_bospectra_files[i].id;
            bospectra_trace_u32("Assigned File ID", g_bospectra_files[i].id);
            bospectra_trace_str("File Open Result", "SUCCESS");
            return BOSPECTRA_SUCCESS;
        }
    }

    vfs_close(fd);
    bospectra_trace_str("File Open Result", "FAILED (BOSPECTRA_ERR_BUFFER_OVERFLOW)");
    return BOSPECTRA_ERR_BUFFER_OVERFLOW;
}

bospectra_error_t bospectra_file_read(bospectra_file_id_t file_id, void* buffer, uint32_t bytes_to_read, uint32_t* out_bytes_read) {
    if (!g_bospectra_file_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!buffer || !out_bytes_read) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (file_id == 0 || file_id > BOSPECTRA_MAX_OPEN_FILES) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = file_id - 1;
    if (!g_bospectra_files[idx].is_open) return BOSPECTRA_ERR_HANDLE_INVALID;

    int bytes_read = vfs_pread(g_bospectra_files[idx].vfs_fd, buffer, bytes_to_read, g_bospectra_files[idx].current_offset);
    if (bytes_read < 0) {
        *out_bytes_read = 0;
        return BOSPECTRA_ERR_FILE_READ_FAILED;
    }

    *out_bytes_read = (uint32_t)bytes_read;
    g_bospectra_files[idx].current_offset += bytes_read;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_file_seek(bospectra_file_id_t file_id, uint64_t target_offset) {
    if (!g_bospectra_file_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (file_id == 0 || file_id > BOSPECTRA_MAX_OPEN_FILES) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = file_id - 1;
    if (!g_bospectra_files[idx].is_open) return BOSPECTRA_ERR_HANDLE_INVALID;

    g_bospectra_files[idx].current_offset = target_offset;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_file_tell(bospectra_file_id_t file_id, uint64_t* out_offset) {
    if (!g_bospectra_file_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!out_offset) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (file_id == 0 || file_id > BOSPECTRA_MAX_OPEN_FILES) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = file_id - 1;
    if (!g_bospectra_files[idx].is_open) return BOSPECTRA_ERR_HANDLE_INVALID;

    *out_offset = g_bospectra_files[idx].current_offset;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_file_get_size(bospectra_file_id_t file_id, uint64_t* out_size) {
    if (!g_bospectra_file_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!out_size) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (file_id == 0 || file_id > BOSPECTRA_MAX_OPEN_FILES) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = file_id - 1;
    if (!g_bospectra_files[idx].is_open) return BOSPECTRA_ERR_HANDLE_INVALID;

    *out_size = g_bospectra_files[idx].file_size;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_file_close(bospectra_file_id_t file_id) {
    if (!g_bospectra_file_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (file_id == 0 || file_id > BOSPECTRA_MAX_OPEN_FILES) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = file_id - 1;
    if (!g_bospectra_files[idx].is_open) return BOSPECTRA_ERR_HANDLE_INVALID;

    vfs_close(g_bospectra_files[idx].vfs_fd);
    memset(&g_bospectra_files[idx], 0, sizeof(BOSPECTRA_FileContext));
    return BOSPECTRA_SUCCESS;
}
