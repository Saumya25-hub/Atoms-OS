#ifndef BOSPECTRA_DEMUX_H
#define BOSPECTRA_DEMUX_H

#include "../../include/bospectra_types.h"
#include "../../packet/bospectra_packet.h"
#include "../../stream/bospectra_stream.h"

typedef uint32_t bospectra_demux_id_t;

typedef struct {
    char     format_name[BOSPECTRA_MAX_NAME_LEN];
    uint64_t duration_us;
    uint32_t stream_count;
    uint32_t video_stream_count;
    uint32_t audio_stream_count;
    uint32_t subtitle_stream_count;
    char     container_brand[32];
} BOSPECTRA_ContainerMetadata;

// Demuxer Session APIs
bospectra_error_t BOSPECTRA_Demux_Open(const char* file_path, bospectra_demux_id_t* out_demux_id);
bospectra_error_t BOSPECTRA_Demux_GetMetadata(bospectra_demux_id_t demux_id, BOSPECTRA_ContainerMetadata* out_meta);
bospectra_error_t BOSPECTRA_Demux_GetStreamDescriptor(bospectra_demux_id_t demux_id, uint32_t stream_index, BOSPECTRA_StreamDescriptor* out_desc);
bospectra_error_t BOSPECTRA_Demux_ReadPacket(bospectra_demux_id_t demux_id, BOSPacket** out_packet);
bospectra_error_t BOSPECTRA_Demux_Seek(bospectra_demux_id_t demux_id, uint64_t timestamp_us);
bospectra_error_t BOSPECTRA_Demux_Close(bospectra_demux_id_t demux_id);

#endif // BOSPECTRA_DEMUX_H
