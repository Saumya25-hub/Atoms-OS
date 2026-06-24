#ifndef ATOM_STRING_H
#define ATOM_STRING_H

#include "atom_value.h"

// The Interned Immutable String
typedef struct AtomString {
    uint32_t magic;      // ATOM_STRING_MAGIC
    uint32_t hash;       // Precomputed FNV-1a hash
    uint32_t length;     // Number of bytes (excluding NUL)
    uint32_t ref_count;  // Reference counter
    char     chars[];    // Flexible array member
} AtomString;

// Core String API
AtomValue atom_string_create(const char* chars);
AtomValue atom_string_create_len(const char* chars, uint32_t length);

void atom_string_retain(AtomString* str);
void atom_string_release(AtomString* str);

// System Management
void atom_string_pool_init(void);
void atom_string_pool_destroy(void);

#endif // ATOM_STRING_H
