#ifndef ATOM_HASH_H
#define ATOM_HASH_H

#include <stdint.h>
#include <stddef.h>

// FNV-1a 32-bit hash constants
#define FNV_PRIME_32 16777619
#define FNV_OFFSET_32 2166136261U

static inline uint32_t atom_hash_bytes(const char* data, size_t length) {
    uint32_t hash = FNV_OFFSET_32;
    for (size_t i = 0; i < length; i++) {
        hash ^= (uint8_t)data[i];
        hash *= FNV_PRIME_32;
    }
    return hash;
}

static inline uint32_t atom_hash_string(const char* str) {
    uint32_t hash = FNV_OFFSET_32;
    while (*str) {
        hash ^= (uint8_t)(*str);
        hash *= FNV_PRIME_32;
        str++;
    }
    return hash;
}

#endif // ATOM_HASH_H
