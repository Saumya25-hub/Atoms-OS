#ifndef MKV_PARSER_H
#define MKV_PARSER_H

#include "../registry/container_registry.h"

// Matroska / WebM EBML Element IDs
#define EBML_ID_HEADER        0x1A45DFA3U
#define EBML_ID_SEGMENT       0x18538067U
#define EBML_ID_SEEKHEAD      0x114D9B74U
#define EBML_ID_INFO          0x1549A966U
#define EBML_ID_TRACKS        0x1654AE6BU
#define EBML_ID_CLUSTER       0x1F43B675U

// Export MKV Driver Struct
extern const BOSPECTRA_ContainerDriver g_mkv_container_driver;

#endif // MKV_PARSER_H
