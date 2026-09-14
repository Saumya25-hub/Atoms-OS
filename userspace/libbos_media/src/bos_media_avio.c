/*
 * ============================================================================
 * ATOMS OS — Native Media AVIO Bridge Implementation
 * userspace/libbos_media/src/bos_media_avio.c
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements real AVIO callbacks backed by BOSMediaStream and ATOMS VFS.
 * ============================================================================
 */

#include "../include/bos_media_avio.h"
#include <string.h>

extern void display_print(const char* s);

static void avio_print_num(uint64_t val) {
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

static void avio_print_signed(int64_t val) {
    if (val < 0) {
        display_print("-");
        avio_print_num((uint64_t)(-val));
    } else {
        avio_print_num((uint64_t)val);
    }
}

static BOSMediaAVIO s_avio_instance __attribute__((aligned(16)));

int bos_media_avio_read_packet(void* opaque, uint8_t* buf, int buf_size) {
    if (!opaque || !buf || buf_size <= 0) {
        display_print("[MEDIA-P3] AVIO_ERROR: Invalid arguments\n");
        return -1;
    }

    BOSMediaStream* s = (BOSMediaStream*)opaque;
    if (s->eof) {
        display_print("[MEDIA-P3] AVIO_EOF\n");
        return 0; // Standard EOF
    }

    int bytes_read = bos_media_stream_read(s, buf, (size_t)buf_size);
    if (bytes_read > 0) {
        display_print("[MEDIA-P3] AVIO_READ: bytes=");
        avio_print_num((uint64_t)bytes_read);
        display_print("\n");
        return bytes_read;
    } else if (bytes_read == 0) {
        display_print("[MEDIA-P3] AVIO_EOF\n");
        return 0;
    } else {
        display_print("[MEDIA-P3] AVIO_ERROR: read failed code=");
        avio_print_signed((int64_t)bytes_read);
        display_print("\n");
        return bytes_read;
    }
}

int64_t bos_media_avio_seek(void* opaque, int64_t offset, int whence) {
    if (!opaque) {
        display_print("[MEDIA-P3] AVIO_ERROR: Null stream in seek\n");
        return -1;
    }

    BOSMediaStream* s = (BOSMediaStream*)opaque;

    // Standard AVIO seek protocol: whence can have AVSEEK_SIZE (0x10000)
    if (whence == 0x10000) {
        return (int64_t)s->size;
    }

    int ret = bos_media_stream_seek(s, offset, whence);
    if (ret == 0) {
        int64_t new_pos = bos_media_stream_tell(s);
        display_print("[MEDIA-P3] AVIO_SEEK: pos=");
        avio_print_signed(new_pos);
        display_print("\n");
        return new_pos;
    }

    display_print("[MEDIA-P3] AVIO_ERROR: seek failed\n");
    return -1;
}

BOSMediaAVIO* bos_media_avio_create(BOSMediaStream* stream) {
    if (!stream) return NULL;

    BOSMediaAVIO* avio = &s_avio_instance;
    memset(avio, 0, sizeof(BOSMediaAVIO));
    avio->stream = stream;
    avio->opaque = stream;
    avio->read_packet = bos_media_avio_read_packet;
    avio->seek = bos_media_avio_seek;

    display_print("[MEDIA-P3] AVIO_CREATE: bound to stream fd=");
    avio_print_num((uint64_t)stream->fd);
    display_print("\n");

    return avio;
}

void bos_media_avio_destroy(BOSMediaAVIO* avio) {
    if (!avio) return;
    display_print("[MEDIA-P3] AVIO_DESTROY\n");
    avio->stream = NULL;
    avio->opaque = NULL;
}
