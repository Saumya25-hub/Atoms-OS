/*
 * ============================================================================
 * ATOMS OS — Native Media AVIO Bridge Interface
 * userspace/libbos_media/include/bos_media_avio.h
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Custom AVIO Bridge mapping upstream demuxer I/O callbacks to BOSMediaStream.
 * ============================================================================
 */

#ifndef BOS_MEDIA_AVIO_H
#define BOS_MEDIA_AVIO_H

#include "bos_media_stream.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BOSMediaAVIO {
    BOSMediaStream* stream;
    uint8_t*        io_buffer;
    size_t          buffer_size;
    
    int     (*read_packet)(void* opaque, uint8_t* buf, int buf_size);
    int64_t (*seek)(void* opaque, int64_t offset, int whence);
    void*   opaque;
} BOSMediaAVIO;

BOSMediaAVIO* bos_media_avio_create(BOSMediaStream* stream);
int           bos_media_avio_read_packet(void* opaque, uint8_t* buf, int buf_size);
int64_t       bos_media_avio_seek(void* opaque, int64_t offset, int whence);
void          bos_media_avio_destroy(BOSMediaAVIO* avio);

#ifdef __cplusplus
}
#endif

#endif /* BOS_MEDIA_AVIO_H */
