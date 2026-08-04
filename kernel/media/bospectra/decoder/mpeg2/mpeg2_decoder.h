#ifndef MPEG2_DECODER_H
#define MPEG2_DECODER_H

#include "../registry/decoder_registry.h"

#define MPEG2_START_CODE_SEQUENCE 0x000001B3U
#define MPEG2_START_CODE_PICTURE  0x00000100U

// Export MPEG2 Decoder Driver Struct
extern const BOSPECTRA_DecoderDriver g_mpeg2_decoder_driver;

#endif // MPEG2_DECODER_H
