#ifndef BOSPECTRA_TYPES_H
#define BOSPECTRA_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Core Handle and Identifier Definitions
typedef uint32_t bospectra_error_t;
typedef uint32_t bospectra_handle_t;
typedef uint32_t bospectra_stream_id_t;
typedef uint32_t bospectra_file_id_t;
typedef uint32_t bospectra_buffer_id_t;

#define BOSPECTRA_INVALID_HANDLE 0x00000000U
#define BOSPECTRA_MAX_NAME_LEN   64U
#define BOSPECTRA_MAX_PATH_LEN   256U

// Engine Lifecycle States
typedef enum {
    BOSPECTRA_STATE_UNINITIALIZED = 0,
    BOSPECTRA_STATE_INITIALIZING,
    BOSPECTRA_STATE_READY,
    BOSPECTRA_STATE_RUNNING,
    BOSPECTRA_STATE_PAUSED,
    BOSPECTRA_STATE_STOPPING,
    BOSPECTRA_STATE_ERROR,
    BOSPECTRA_STATE_SHUTDOWN
} bospectra_state_t;

// Stream Types
typedef enum {
    BOSPECTRA_STREAM_UNKNOWN = 0,
    BOSPECTRA_STREAM_VIDEO,
    BOSPECTRA_STREAM_AUDIO,
    BOSPECTRA_STREAM_SUBTITLE,
    BOSPECTRA_STREAM_DATA
} bospectra_stream_type_t;

// Engine Semantic Versioning Structure
typedef struct {
    uint8_t  major;
    uint8_t  minor;
    uint8_t  patch;
    uint16_t build;
    char     build_tag[BOSPECTRA_MAX_NAME_LEN];
} BOSPECTRA_Version;

#endif // BOSPECTRA_TYPES_H
