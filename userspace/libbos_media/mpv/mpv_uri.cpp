/*
 * ============================================================================
 * ATOMS OS — BOS libmpv Stream / VFS Bridge
 * userspace/libbos_media/mpv/mpv_uri.cpp
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Translates libmpv stream requests into native ATOMS VFS syscalls.
 * Zero POSIX fopen / open / Win32 dependencies.
 * ============================================================================
 */

#include "mpv_adapter.h"
#include <string.h>

typedef struct {
    int     fd;
    int64_t file_size;
    int64_t current_pos;
} VfsStreamCookie;

static int64_t vfs_stream_read(void* cookie, char* buf, uint64_t nbytes) {
    if (!cookie || !buf || nbytes == 0) return 0;
    VfsStreamCookie* s = (VfsStreamCookie*)cookie;
    if (s->fd < 0) return -1;

    int64_t read_bytes = (int64_t)__atoms_syscall3(SYS_READ, (uint64_t)s->fd, (uint64_t)buf, (uint64_t)nbytes);
    if (read_bytes > 0) {
        s->current_pos += read_bytes;
    }
    return read_bytes;
}

static int64_t vfs_stream_seek(void* cookie, int64_t offset) {
    if (!cookie) return -1;
    VfsStreamCookie* s = (VfsStreamCookie*)cookie;
    if (s->fd < 0) return -1;

    int64_t res = (int64_t)__atoms_syscall3(SYS_SEEK, (uint64_t)s->fd, (uint64_t)offset, 0 /* SEEK_SET */);
    if (res >= 0) {
        s->current_pos = res;
    }
    return res;
}

static int64_t vfs_stream_size(void* cookie) {
    if (!cookie) return -1;
    VfsStreamCookie* s = (VfsStreamCookie*)cookie;
    return s->file_size;
}

static void vfs_stream_close(void* cookie) {
    if (!cookie) return;
    VfsStreamCookie* s = (VfsStreamCookie*)cookie;
    if (s->fd >= 0) {
        __atoms_syscall1(SYS_CLOSE, (uint64_t)s->fd);
        s->fd = -1;
    }
    delete s;
}

int mpv_vfs_stream_open(void* user_data, char* uri, mpv_stream_cb_info* info) {
    if (!info || !uri) return MPV_ERROR_INVALID_PARAMETER;
    BOSMpvAdapter* adapter = (BOSMpvAdapter*)user_data;

    // Sanitize URI and extract VFS path
    // Supports:
    //   atoms://storage/USB0/TEST.MP4 -> /volumes/usb0/TEST.MP4
    //   bofs://...
    //   /volumes/usb0/...
    //   /TEST.MP4
    char vfs_path[256];
    vfs_path[0] = '\0';

    if (strncmp(uri, "atoms://storage/USB0/", 21) == 0) {
        strcpy(vfs_path, "/volumes/usb0/");
        strcat(vfs_path, uri + 21);
    } else if (strncmp(uri, "atoms://storage/usb0/", 21) == 0) {
        strcpy(vfs_path, "/volumes/usb0/");
        strcat(vfs_path, uri + 21);
    } else if (strncmp(uri, "atoms://", 8) == 0) {
        strcpy(vfs_path, "/");
        strcat(vfs_path, uri + 8);
    } else if (strncmp(uri, "bofs://", 7) == 0) {
        strcpy(vfs_path, "/");
        strcat(vfs_path, uri + 7);
    } else if (strncmp(uri, "file://", 7) == 0) {
        strcpy(vfs_path, uri + 7);
    } else {
        // Direct local or relative path
        if (uri[0] != '/') {
            vfs_path[0] = '/';
            strcpy(vfs_path + 1, uri);
        } else {
            strcpy(vfs_path, uri);
        }
    }

    // Open through ATOMS VFS syscall
    int fd = (int)__atoms_syscall2(SYS_OPEN, (uint64_t)vfs_path, 0 /* O_RDONLY */);
    if (fd < 0) {
        // Fallback search: try /volumes/usb0 if root open fails
        if (strncmp(vfs_path, "/volumes/usb0/", 14) != 0 && vfs_path[0] == '/') {
            char usb_alt[256];
            strcpy(usb_alt, "/volumes/usb0");
            strcat(usb_alt, vfs_path);
            fd = (int)__atoms_syscall2(SYS_OPEN, (uint64_t)usb_alt, 0);
            if (fd >= 0) {
                strcpy(vfs_path, usb_alt);
            }
        }
    }

    if (fd < 0) {
        if (adapter) {
            adapter->telemetry.first_failure_stage = "STREAM_OPEN";
            adapter->telemetry.first_failure_reason = "FILE_NOT_FOUND_IN_VFS";
        }
        return MPV_ERROR_LOADING_FAILED;
    }

    // Determine file size via seek to end
    int64_t size = (int64_t)__atoms_syscall3(SYS_SEEK, (uint64_t)fd, 0, 2 /* SEEK_END */);
    __atoms_syscall3(SYS_SEEK, (uint64_t)fd, 0, 0 /* SEEK_SET */);

    VfsStreamCookie* cookie = new VfsStreamCookie();
    cookie->fd = fd;
    cookie->file_size = (size >= 0) ? size : 0;
    cookie->current_pos = 0;

    info->cookie = cookie;
    info->read = vfs_stream_read;
    info->seek = vfs_stream_seek;
    info->size = vfs_stream_size;
    info->close = vfs_stream_close;
    info->cancel = nullptr;

    if (adapter) {
        strcpy(adapter->current_uri, vfs_path);
        adapter->vfs_fd = fd;
        adapter->file_size = cookie->file_size;
    }

    return MPV_ERROR_SUCCESS;
}
