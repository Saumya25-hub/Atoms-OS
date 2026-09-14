#ifndef BOSPECTRA_STREAM_H
#define BOSPECTRA_STREAM_H

#include "../include/bospectra_types.h"

#define BOSPECTRA_MAX_STREAMS 16U

typedef struct {
    bospectra_stream_id_t   id;
    bospectra_stream_type_t type;
    char                    codec_name[BOSPECTRA_MAX_NAME_LEN];
    uint32_t                codec_id;
    uint32_t                bitrate;
    uint32_t                width;          // Video streams only
    uint32_t                height;         // Video streams only
    uint32_t                frame_rate_num; // Video streams only
    uint32_t                frame_rate_den; // Video streams only
    uint32_t                sample_rate;    // Audio streams only
    uint8_t                 channels;       // Audio streams only
    bool                    is_active;
    const void*             extradata;      // Decoder config (avcC, SPS/PPS)
    uint32_t                extradata_size;
} BOSPECTRA_StreamDescriptor;

void              bospectra_stream_subsystem_init(void);
void              bospectra_stream_subsystem_shutdown(void);
bospectra_error_t bospectra_stream_register(const BOSPECTRA_StreamDescriptor* desc, bospectra_stream_id_t* out_id);
bospectra_error_t bospectra_stream_unregister(bospectra_stream_id_t stream_id);
bospectra_error_t bospectra_stream_get_descriptor(bospectra_stream_id_t stream_id, BOSPECTRA_StreamDescriptor* out_desc);
uint32_t          bospectra_stream_get_count(void);

#endif // BOSPECTRA_STREAM_H
