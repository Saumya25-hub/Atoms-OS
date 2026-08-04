#ifndef H264_DECODER_H
#define H264_DECODER_H

#include "../registry/decoder_registry.h"

#define H264_NAL_TYPE_SLICE 1U
#define H264_NAL_TYPE_IDR   5U
#define H264_NAL_TYPE_SEI   6U
#define H264_NAL_TYPE_SPS   7U
#define H264_NAL_TYPE_PPS   8U

// Export H.264 Decoder Driver Struct
extern const BOSPECTRA_DecoderDriver g_h264_decoder_driver;

#endif // H264_DECODER_H
