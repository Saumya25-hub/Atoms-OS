/*
 * ============================================================================
 * ATOMS OS — Native Userspace Media Stream Interface
 * userspace/libbos_media/include/bos_media_stream.h
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Production Freestanding VFS Stream Abstraction for Demuxers & Codecs.
 * Operates strictly via ATOMS VFS Syscalls: SYS_OPEN, SYS_READ, SYS_SEEK, SYS_CLOSE.
 * Zero POSIX, zero Linux, zero Windows file dependencies.
 * ============================================================================
 */

#ifndef BOS_MEDIA_STREAM_H
#define BOS_MEDIA_STREAM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOS_SEEK_SET 0
#define BOS_SEEK_CUR 1
#define BOS_SEEK_END 2

#define BOS_STREAM_CACHE_SIZE (64 * 1024)

typedef struct BOSMediaStream {
    int      fd;
    uint64_t size;
    uint64_t position;
    bool     eof;
    int      error_code;
    bool     in_use;
    
    // 64 KB Read Cache Buffer
    uint8_t  cache[BOS_STREAM_CACHE_SIZE];
    uint64_t cache_offset;
    size_t   cache_valid_bytes;

    // Performance & Forensic Metrics
    uint64_t stat_cache_hits;
    uint64_t stat_cache_misses;
    uint64_t stat_refills;
    uint64_t stat_bytes_read;
    uint64_t stat_syscalls;
    
    // Function Pointers for Standard Interoperability
    int      (*read)(struct BOSMediaStream* s, void* buffer, size_t count);
    int      (*seek)(struct BOSMediaStream* s, int64_t offset, int whence);
    int64_t  (*tell)(struct BOSMediaStream* s);
    uint64_t (*size_fn)(struct BOSMediaStream* s);
    int      (*eof_fn)(struct BOSMediaStream* s);
    int      (*error_fn)(struct BOSMediaStream* s);
    void     (*close)(struct BOSMediaStream* s);
} BOSMediaStream;

BOSMediaStream* bos_media_stream_open(const char* uri);
int             bos_media_stream_read(BOSMediaStream* s, void* buffer, size_t count);
int             bos_media_stream_seek(BOSMediaStream* s, int64_t offset, int whence);
int64_t         bos_media_stream_tell(BOSMediaStream* s);
uint64_t        bos_media_stream_size(BOSMediaStream* s);
int             bos_media_stream_eof(BOSMediaStream* s);
int             bos_media_stream_error(BOSMediaStream* s);
void            bos_media_stream_close(BOSMediaStream* stream);

#ifdef __cplusplus
}
#endif

#endif /* BOS_MEDIA_STREAM_H */
