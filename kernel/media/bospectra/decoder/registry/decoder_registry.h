#ifndef DECODER_REGISTRY_H
#define DECODER_REGISTRY_H

#include "../include/bospectra_codec_types.h"
#include "../../stream/bospectra_stream.h"
#include "../../packet/bospectra_packet.h"
#include "../../frame_memory/include/bospectra_frame.h"
#include "../../include/bospectra_errors.h"

// Polymorphic Codec Decoder Driver vtable
typedef struct BOSPECTRA_DecoderDriver {
    const char*          codec_name;
    bospectra_codec_id_t codec_id;

    // Open decoder instance for target stream descriptor
    bospectra_error_t (*open)(void** driver_ctx, const BOSPECTRA_StreamDescriptor* stream_desc);

    // Decode compressed packet -> Output raw uncompressed BOSFrame
    bospectra_error_t (*decode_packet)(void* driver_ctx, const BOSPacket* packet, BOSFrame** out_frame);

    // Flush reference frames & bitstream buffers during seek
    bospectra_error_t (*flush)(void* driver_ctx);

    // Close decoder instance & release context
    bospectra_error_t (*close)(void* driver_ctx);
} BOSPECTRA_DecoderDriver;

void                             bospectra_decoder_registry_init(void);
void                             bospectra_decoder_registry_shutdown(void);
bospectra_error_t                bospectra_decoder_register_driver(const BOSPECTRA_DecoderDriver* driver);
const BOSPECTRA_DecoderDriver* bospectra_decoder_find_driver(bospectra_codec_id_t codec_id);
const BOSPECTRA_DecoderDriver* bospectra_decoder_find_driver_by_name(const char* codec_name);

#endif // DECODER_REGISTRY_H
