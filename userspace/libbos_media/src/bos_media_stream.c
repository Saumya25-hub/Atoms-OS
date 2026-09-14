/*
 * ============================================================================
 * ATOMS OS — Native Userspace Media Stream Implementation
 * userspace/libbos_media/src/bos_media_stream.c
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements BOSMediaStream backed by ATOMS VFS Syscalls with 64 KB Read Cache.
 * ============================================================================
 */

#include "../include/bos_media_stream.h"
#include "userspace/runtime/c/include/atoms_syscall.h"
#include <string.h>

extern void display_print(const char* s);

static void p3_print_num(uint64_t val) {
    char buf[24];
    int p = 0;
    if (val == 0) {
        display_print("0");
        return;
    }
    char tmp[24];
    int tp = 0;
    while (val > 0) {
        tmp[tp++] = '0' + (val % 10);
        val /= 10;
    }
    while (tp > 0) {
        buf[p++] = tmp[--tp];
    }
    buf[p] = '\0';
    display_print(buf);
}

static void p3_print_signed(int64_t val) {
    if (val < 0) {
        display_print("-");
        p3_print_num((uint64_t)(-val));
    } else {
        p3_print_num((uint64_t)val);
    }
}

#define MAX_STREAM_INSTANCES 4
static BOSMediaStream s_stream_pool[MAX_STREAM_INSTANCES] __attribute__((aligned(16)));

static int vfs_stream_read(BOSMediaStream* s, void* buffer, size_t count) {
    if (!s || s->fd < 0 || !buffer || count == 0) return 0;
    if (s->eof) return 0;

    uint8_t* out = (uint8_t*)buffer;
    size_t total_copied = 0;

    while (total_copied < count) {
        // Check if current stream position is within cache
        if (s->cache_valid_bytes > 0 &&
            s->position >= s->cache_offset &&
            s->position < s->cache_offset + s->cache_valid_bytes) {

            size_t cache_avail = (size_t)((s->cache_offset + s->cache_valid_bytes) - s->position);
            size_t needed = count - total_copied;
            size_t to_copy = (needed < cache_avail) ? needed : cache_avail;

            size_t cache_idx = (size_t)(s->position - s->cache_offset);
            memcpy(out + total_copied, s->cache + cache_idx, to_copy);

            s->position += to_copy;
            total_copied += to_copy;
            s->stat_cache_hits++;
            s->stat_bytes_read += to_copy;
            continue;
        }

        // Cache Miss: determine strategy
        s->stat_cache_misses++;
        size_t remaining = count - total_copied;

        if (remaining >= (BOS_STREAM_CACHE_SIZE / 2)) {
            // Bulk read bypasses cache to avoid double-copy overhead
            s->stat_syscalls++;
            int64_t res = (int64_t)__atoms_syscall3(SYS_READ, (uint64_t)s->fd, (uint64_t)(out + total_copied), (uint64_t)remaining);
            if (res > 0) {
                s->position += (uint64_t)res;
                total_copied += (size_t)res;
                s->stat_bytes_read += (size_t)res;
                // Invalidate cache since position advanced past cache
                s->cache_valid_bytes = 0;
            } else if (res == 0) {
                s->eof = true;
                display_print("[MEDIA-P3] EOF_STATE=1\n");
                break;
            } else {
                s->error_code = (int)res;
                break;
            }
        } else {
            // Small read: Refill full 64 KB cache from kernel VFS
            s->stat_syscalls++;
            s->stat_refills++;
            display_print("[MEDIA-P3] VFS_REFILL: pos=");
            p3_print_num(s->position);
            display_print("\n");

            // Align hardware file cursor with s->position before refill
            __atoms_syscall3(SYS_SEEK, (uint64_t)s->fd, (uint64_t)s->position, BOS_SEEK_SET);
            int64_t rd = (int64_t)__atoms_syscall3(SYS_READ, (uint64_t)s->fd, (uint64_t)s->cache, BOS_STREAM_CACHE_SIZE);
            if (rd > 0) {
                s->cache_offset = s->position;
                s->cache_valid_bytes = (size_t)rd;
            } else if (rd == 0) {
                s->eof = true;
                s->cache_valid_bytes = 0;
                display_print("[MEDIA-P3] EOF_STATE=1\n");
                break;
            } else {
                s->error_code = (int)rd;
                s->cache_valid_bytes = 0;
                break;
            }
        }
    }

    return (int)total_copied;
}

static int vfs_stream_seek(BOSMediaStream* s, int64_t offset, int whence) {
    if (!s || s->fd < 0) return -1;

    display_print("[MEDIA-P3] SEEK_REQUEST: off=");
    p3_print_signed(offset);
    display_print(" whence=");
    p3_print_num((uint64_t)whence);
    display_print("\n");

    int64_t target = 0;
    if (whence == BOS_SEEK_SET) {
        target = offset;
    } else if (whence == BOS_SEEK_CUR) {
        target = (int64_t)s->position + offset;
    } else if (whence == BOS_SEEK_END) {
        target = (int64_t)s->size + offset;
    } else {
        return -1;
    }

    if (target < 0) target = 0;

    // Fast-path: Check if target position is still within existing cache
    if (s->cache_valid_bytes > 0 &&
        (uint64_t)target >= s->cache_offset &&
        (uint64_t)target < s->cache_offset + s->cache_valid_bytes) {
        s->position = (uint64_t)target;
        s->stat_cache_hits++;
        display_print("[MEDIA-P3] VFS_CACHE_HIT\n");
    } else {
        // Cache miss: sync kernel VFS cursor and invalidate cache
        uint64_t res = __atoms_syscall3(SYS_SEEK, (uint64_t)s->fd, (uint64_t)target, BOS_SEEK_SET);
        s->position = res;
        s->cache_valid_bytes = 0;
        s->stat_syscalls++;
    }

    if (s->position < s->size) {
        s->eof = false;
    }

    display_print("[MEDIA-P3] SEEK_RESULT: pos=");
    p3_print_num(s->position);
    display_print("\n");
    display_print("[MEDIA-P3] STREAM_POSITION: ");
    p3_print_num(s->position);
    display_print("\n");

    return 0;
}

static int64_t vfs_stream_tell(BOSMediaStream* s) {
    if (!s || s->fd < 0) return -1;
    return (int64_t)s->position;
}

static uint64_t vfs_stream_size_fn(BOSMediaStream* s) {
    if (!s || s->fd < 0) return 0;
    return s->size;
}

static int vfs_stream_eof_fn(BOSMediaStream* s) {
    if (!s) return 1;
    return s->eof ? 1 : 0;
}

static int vfs_stream_error_fn(BOSMediaStream* s) {
    if (!s) return -1;
    return s->error_code;
}

static void vfs_stream_close(BOSMediaStream* s) {
    if (!s || s->fd < 0) return;
    display_print("[MEDIA-P3] VFS_CLOSE: fd=");
    p3_print_num((uint64_t)s->fd);
    display_print("\n");

    __atoms_syscall1(SYS_CLOSE, (uint64_t)s->fd);
    s->fd = -1;
    s->position = 0;
    s->size = 0;
    s->cache_valid_bytes = 0;
    s->in_use = false;
}

BOSMediaStream* bos_media_stream_open(const char* uri) {
    if (!uri) return NULL;
    
    display_print("[MEDIA-P3] VFS_OPEN: uri=");
    display_print(uri);
    
    int fd = (int)__atoms_syscall2(SYS_OPEN, (uint64_t)uri, 0);
    if (fd < 0) {
        display_print(" -> FAIL fd=");
        p3_print_signed((int64_t)fd);
        display_print("\n");
        return NULL;
    }
    
    display_print(" -> OK fd=");
    p3_print_num((uint64_t)fd);
    display_print("\n");

    // Allocate an available stream slot from pool
    BOSMediaStream* s = NULL;
    for (int i = 0; i < MAX_STREAM_INSTANCES; i++) {
        if (!s_stream_pool[i].in_use) {
            s = &s_stream_pool[i];
            break;
        }
    }

    if (!s) {
        display_print("[MEDIA-P3] VFS_OPEN: Error stream pool exhausted\n");
        __atoms_syscall1(SYS_CLOSE, (uint64_t)fd);
        return NULL;
    }

    memset(s, 0, sizeof(BOSMediaStream));
    s->in_use = true;
    s->fd = fd;
    s->read = vfs_stream_read;
    s->seek = vfs_stream_seek;
    s->tell = vfs_stream_tell;
    s->size_fn = vfs_stream_size_fn;
    s->eof_fn = vfs_stream_eof_fn;
    s->error_fn = vfs_stream_error_fn;
    s->close = vfs_stream_close;

    // Determine stream size via seek to end
    uint64_t sz = __atoms_syscall3(SYS_SEEK, (uint64_t)fd, 0, BOS_SEEK_END);
    s->size = sz;
    // Seek back to start
    __atoms_syscall3(SYS_SEEK, (uint64_t)fd, 0, BOS_SEEK_SET);
    s->position = 0;

    display_print("[MEDIA-P3] STREAM_SIZE=");
    p3_print_num(s->size);
    display_print(" bytes\n");

    return s;
}

int bos_media_stream_read(BOSMediaStream* s, void* buffer, size_t count) {
    if (s && s->read) return s->read(s, buffer, count);
    return -1;
}

int bos_media_stream_seek(BOSMediaStream* s, int64_t offset, int whence) {
    if (s && s->seek) return s->seek(s, offset, whence);
    return -1;
}

int64_t bos_media_stream_tell(BOSMediaStream* s) {
    if (s && s->tell) return s->tell(s);
    return -1;
}

uint64_t bos_media_stream_size(BOSMediaStream* s) {
    if (s && s->size_fn) return s->size_fn(s);
    return s ? s->size : 0;
}

int bos_media_stream_eof(BOSMediaStream* s) {
    if (s && s->eof_fn) return s->eof_fn(s);
    return 1;
}

int bos_media_stream_error(BOSMediaStream* s) {
    if (s && s->error_fn) return s->error_fn(s);
    return -1;
}

void bos_media_stream_close(BOSMediaStream* stream) {
    if (stream && stream->close) {
        stream->close(stream);
    }
}
