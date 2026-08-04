/*
 * BOSPECTRA V3 — Dynamic Codec Registry
 * kernel/media/bospectra/registry/codec_registry.h
 *
 * Single source of truth for video/audio codec registration, capability matching, and resolution.
 */

#ifndef BOSPECTRA_V3_CODEC_REGISTRY_H
#define BOSPECTRA_V3_CODEC_REGISTRY_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include "../decoder/include/bospectra_decoder.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BOSPECTRA_MAX_CODEC_DRIVERS 16U

typedef struct {
    bospectra_codec_id_t codec_id;
    const char*          codec_name;
    uint32_t             priority;
    bospectra_error_t (*open)(void** driver_ctx, const BOSPECTRA_StreamDescriptor* stream_desc);
    bospectra_error_t (*decode_packet)(void* driver_ctx, const BOSPacket* packet, BOSFrame** out_frame);
    bospectra_error_t (*flush)(void* driver_ctx);
    bospectra_error_t (*close)(void* driver_ctx);
} BOSPECTRA_CodecDriverRecord;

void bospectra_v3_codec_registry_init(void);
void bospectra_v3_codec_registry_shutdown(void);

bospectra_error_t bospectra_v3_codec_register(const BOSPECTRA_CodecDriverRecord* driver);
const BOSPECTRA_CodecDriverRecord* bospectra_v3_codec_find_by_name(const char* name);
const BOSPECTRA_CodecDriverRecord* bospectra_v3_codec_find_by_id(bospectra_codec_id_t codec_id);
uint32_t bospectra_v3_codec_get_registered(const BOSPECTRA_CodecDriverRecord** out_drivers, uint32_t max_count);

#endif /* BOSPECTRA_V3_CODEC_REGISTRY_H */
