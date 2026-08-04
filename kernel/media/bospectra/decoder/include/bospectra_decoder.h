#ifndef BOSPECTRA_DECODER_H
#define BOSPECTRA_DECODER_H

#include "../../include/bospectra_types.h"
#include "../../packet/bospectra_packet.h"
#include "../../stream/bospectra_stream.h"
#include "../../frame_memory/include/bospectra_frame.h"
#include "bospectra_codec_types.h"

typedef uint32_t bospectra_decoder_id_t;

typedef struct {
    uint64_t total_frames_decoded;
    uint64_t total_packets_processed;
    uint64_t total_decode_failures;
    uint64_t last_decode_time_us;
    uint64_t avg_decode_time_us;
    uint64_t peak_decode_time_us;
    char     active_codec_name[32];
} BOSPECTRA_DecoderStats;

// Master Decoder Engine Lifecycle APIs
bospectra_error_t BOSPECTRA_Decoder_Init(void);
bospectra_error_t BOSPECTRA_Decoder_Shutdown(void);

// Decoder Session APIs
bospectra_error_t BOSPECTRA_Decoder_Open(const BOSPECTRA_StreamDescriptor* stream_desc, bospectra_decoder_id_t* out_decoder_id);
bospectra_error_t BOSPECTRA_Decoder_DecodePacket(bospectra_decoder_id_t decoder_id, const BOSPacket* packet, BOSFrame** out_frame);
bospectra_error_t BOSPECTRA_Decoder_Flush(bospectra_decoder_id_t decoder_id);
bospectra_error_t BOSPECTRA_Decoder_Close(bospectra_decoder_id_t decoder_id);

// Telemetry Query
void BOSPECTRA_Decoder_GetStats(BOSPECTRA_DecoderStats* out_stats);
void BOSPECTRA_Decoder_DumpDiagnostics(void);

#endif // BOSPECTRA_DECODER_H
