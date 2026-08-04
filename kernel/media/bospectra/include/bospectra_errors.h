#ifndef BOSPECTRA_ERRORS_H
#define BOSPECTRA_ERRORS_H

#include "bospectra_types.h"

// Deterministic Error Codes (0x8B05XXXX Prefix - 8 = Error, B05 = BOSpectra)
#define BOSPECTRA_SUCCESS                      0x00000000U
#define BOSPECTRA_ERR_NOT_INITIALIZED          0x8B050001U
#define BOSPECTRA_ERR_ALREADY_INITIALIZED      0x8B050002U
#define BOSPECTRA_ERR_INVALID_ARGUMENT         0x8B050003U
#define BOSPECTRA_ERR_OUT_OF_MEMORY            0x8B050004U
#define BOSPECTRA_ERR_STATE_INVALID            0x8B050005U
#define BOSPECTRA_ERR_FILE_NOT_FOUND           0x8B050006U
#define BOSPECTRA_ERR_FILE_READ_FAILED         0x8B050007U
#define BOSPECTRA_ERR_BUFFER_OVERFLOW          0x8B050008U
#define BOSPECTRA_ERR_BUFFER_UNDERFLOW         0x8B050009U
#define BOSPECTRA_ERR_HANDLE_INVALID           0x8B05000AU
#define BOSPECTRA_ERR_STREAM_EXISTS            0x8B05000BU
#define BOSPECTRA_ERR_STREAM_NOT_FOUND         0x8B05000CU
#define BOSPECTRA_ERR_SUBSYSTEM_FAILED         0x8B05000DU
#define BOSPECTRA_ERR_SELF_TEST_FAILED         0x8B05000EU
#define BOSPECTRA_ERR_UNSUPPORTED_FORMAT       0x8B05000FU

// Helper function to map error codes to human-readable diagnostic strings
const char* bospectra_error_to_string(bospectra_error_t err);

#endif // BOSPECTRA_ERRORS_H
