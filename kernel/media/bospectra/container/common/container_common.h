#ifndef CONTAINER_COMMON_H
#define CONTAINER_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// FourCC Macro Definition (32-bit Integer Encoding)
#define BOSPECTRA_FOURCC(a, b, c, d) \
    (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(c) << 8) | (uint32_t)(d))

// Safe Big-Endian to Host Byte Swapping Helpers
uint16_t bospectra_read_u16_be(const uint8_t* ptr);
uint32_t bospectra_read_u32_be(const uint8_t* ptr);
uint64_t bospectra_read_u64_be(const uint8_t* ptr);

// Safe Little-Endian to Host Byte Swapping Helpers
uint16_t bospectra_read_u16_le(const uint8_t* ptr);
uint32_t bospectra_read_u32_le(const uint8_t* ptr);
uint64_t bospectra_read_u64_le(const uint8_t* ptr);

// Boundary & Overflow Check Helper
bool bospectra_container_check_bounds(uint64_t current_offset, uint64_t bytes_to_read, uint64_t total_file_size);

#endif // CONTAINER_COMMON_H
