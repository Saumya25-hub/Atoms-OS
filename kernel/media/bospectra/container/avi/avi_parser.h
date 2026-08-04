#ifndef AVI_PARSER_H
#define AVI_PARSER_H

#include "../registry/container_registry.h"

// RIFF / AVI Chunk FourCC Identifiers
#define AVI_FOURCC_RIFF BOSPECTRA_FOURCC('R', 'I', 'F', 'F')
#define AVI_FOURCC_AVI  BOSPECTRA_FOURCC('A', 'V', 'I', ' ')
#define AVI_FOURCC_LIST BOSPECTRA_FOURCC('L', 'I', 'S', 'T')
#define AVI_FOURCC_HDRL BOSPECTRA_FOURCC('h', 'd', 'r', 'l')
#define AVI_FOURCC_AVIH BOSPECTRA_FOURCC('a', 'v', 'i', 'h')
#define AVI_FOURCC_STRL BOSPECTRA_FOURCC('s', 't', 'r', 'l')
#define AVI_FOURCC_STRH BOSPECTRA_FOURCC('s', 't', 'r', 'h')
#define AVI_FOURCC_STRF BOSPECTRA_FOURCC('s', 't', 'r', 'f')
#define AVI_FOURCC_MOVI BOSPECTRA_FOURCC('m', 'o', 'v', 'i')
#define AVI_FOURCC_IDX1 BOSPECTRA_FOURCC('i', 'd', 'x', '1')

// Stream Types
#define AVI_FOURCC_VIDS BOSPECTRA_FOURCC('v', 'i', 'd', 's')
#define AVI_FOURCC_AUDS BOSPECTRA_FOURCC('a', 'u', 'd', 's')
#define AVI_FOURCC_TXTS BOSPECTRA_FOURCC('t', 'x', 't', 's')

// Export AVI Driver Struct
extern const BOSPECTRA_ContainerDriver g_avi_container_driver;

#endif // AVI_PARSER_H
