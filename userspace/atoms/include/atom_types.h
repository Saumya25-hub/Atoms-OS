#ifndef ATOM_TYPES_H
#define ATOM_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Core Type Tags
typedef enum {
    ATOM_TYPE_NIL       = 0x00,
    ATOM_TYPE_BOOL      = 0x01,
    ATOM_TYPE_NUMBER    = 0x02,  // Unified Number (int or double internally)
    ATOM_TYPE_STRING    = 0x03,  // Interned immutable string
    ATOM_TYPE_ARRAY     = 0x04,  // Dynamic Array
    ATOM_TYPE_TABLE     = 0x05,  // Hash Table
    ATOM_TYPE_OBJECT    = 0x06,  // Opaque user-defined object pointer (V2+)
    ATOM_TYPE_ERROR     = 0xFF   // Error sentinel
} AtomType;

// Forward declarations for complex types
typedef struct AtomString AtomString;
typedef struct AtomArray AtomArray;
typedef struct AtomTable AtomTable;

// Magic Numbers for safety
#define ATOM_STRING_MAGIC 0xA70FACE5
#define ATOM_ARRAY_MAGIC  0xA70A44AE
#define ATOM_TABLE_MAGIC  0xA70TA81E
#define ATOM_FREED_MAGIC  0xDEADDEAD

// Utility to get human-readable name of a type
const char* atom_type_to_string(AtomType type);

#endif // ATOM_TYPES_H
