#ifndef BOS_LOADER_TYPES_H
#define BOS_LOADER_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Status Return Codes for Loader Operations
typedef enum {
    LOADER_SUCCESS                   =  0,
    LOADER_ERR_NULL_POINTER          = -1,
    LOADER_ERR_INVALID_MAGIC         = -2,
    LOADER_ERR_INVALID_ARCH          = -3,
    LOADER_ERR_INVALID_ENDIAN        = -4,
    LOADER_ERR_INVALID_VERSION       = -5,
    LOADER_ERR_INVALID_HEADER_SIZE   = -6,
    LOADER_ERR_CORRUPT_SEGMENT       = -7,
    LOADER_ERR_NO_MEMORY             = -8,
    LOADER_ERR_FILE_NOT_FOUND        = -9,
    LOADER_ERR_SYMBOL_NOT_FOUND      = -10,
    LOADER_ERR_UNSUPPORTED_RELOC     = -11,
    LOADER_ERR_CIRCULAR_DEPENDENCY   = -12,
    LOADER_ERR_MAX_LIBRARIES_REACHED = -13,
    LOADER_ERR_VERIFICATION_FAILED   = -14
} loader_status_t;

// Linker Modes for dlopen()
typedef enum {
    RTLD_LAZY = 0x0001,
    RTLD_NOW  = 0x0002,
    RTLD_GLOBAL = 0x0100,
    RTLD_LOCAL  = 0x0000
} rtld_flags_t;

// Opaque Handle for Shared Libraries
typedef void* library_handle_t;

// Symbol Bindings
typedef enum {
    SYM_BIND_LOCAL  = 0,
    SYM_BIND_GLOBAL = 1,
    SYM_BIND_WEAK   = 2
} symbol_bind_t;

// Symbol Types
typedef enum {
    SYM_TYPE_NOTYPE  = 0,
    SYM_TYPE_OBJECT  = 1,
    SYM_TYPE_FUNC    = 2,
    SYM_TYPE_SECTION = 3,
    SYM_TYPE_FILE    = 4
} symbol_type_t;

// Symbol Export Descriptor
typedef struct {
    const char*   name;
    void*         address;
    uint64_t      size;
    symbol_bind_t binding;
    symbol_type_t type;
} loader_symbol_t;

#endif // BOS_LOADER_TYPES_H
