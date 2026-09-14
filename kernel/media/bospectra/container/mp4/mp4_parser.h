#ifndef MP4_PARSER_H
#define MP4_PARSER_H

#include "../registry/container_registry.h"
#include "../common/container_common.h"

// ISO BMFF Box FourCC Identifiers
#define MP4_BOX_FTYP BOSPECTRA_FOURCC('f', 't', 'y', 'p')
#define MP4_BOX_MOOV BOSPECTRA_FOURCC('m', 'o', 'o', 'v')
#define MP4_BOX_MVHD BOSPECTRA_FOURCC('m', 'v', 'h', 'd')
#define MP4_BOX_TRAK BOSPECTRA_FOURCC('t', 'r', 'a', 'k')
#define MP4_BOX_TKHD BOSPECTRA_FOURCC('t', 'k', 'h', 'd')
#define MP4_BOX_MDIA BOSPECTRA_FOURCC('m', 'd', 'i', 'a')
#define MP4_BOX_MDHD BOSPECTRA_FOURCC('m', 'd', 'h', 'd')
#define MP4_BOX_HDLR BOSPECTRA_FOURCC('h', 'd', 'l', 'r')
#define MP4_BOX_MINF BOSPECTRA_FOURCC('m', 'i', 'n', 'f')
#define MP4_BOX_STBL BOSPECTRA_FOURCC('s', 't', 'b', 'l')
#define MP4_BOX_STSD BOSPECTRA_FOURCC('s', 't', 's', 'd')
#define MP4_BOX_STTS BOSPECTRA_FOURCC('s', 't', 't', 's')
#define MP4_BOX_STSC BOSPECTRA_FOURCC('s', 't', 's', 'c')
#define MP4_BOX_STSZ BOSPECTRA_FOURCC('s', 't', 's', 'z')
#define MP4_BOX_STCO BOSPECTRA_FOURCC('s', 't', 'c', 'o')
#define MP4_BOX_CO64 BOSPECTRA_FOURCC('c', 'o', '6', '4')
#define MP4_BOX_MDAT BOSPECTRA_FOURCC('m', 'd', 'a', 't')

// Export MP4 Driver Struct
extern const BOSPECTRA_ContainerDriver g_mp4_container_driver;

#endif // MP4_PARSER_H
