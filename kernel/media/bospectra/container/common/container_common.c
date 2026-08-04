#include "container_common.h"

uint16_t bospectra_read_u16_be(const uint8_t* ptr) {
    if (!ptr) return 0;
    return ((uint16_t)ptr[0] << 8) | ((uint16_t)ptr[1]);
}

uint32_t bospectra_read_u32_be(const uint8_t* ptr) {
    if (!ptr) return 0;
    return ((uint32_t)ptr[0] << 24) | ((uint32_t)ptr[1] << 16) |
           ((uint32_t)ptr[2] << 8)  | ((uint32_t)ptr[3]);
}

uint64_t bospectra_read_u64_be(const uint8_t* ptr) {
    if (!ptr) return 0;
    return ((uint64_t)ptr[0] << 56) | ((uint64_t)ptr[1] << 48) |
           ((uint64_t)ptr[2] << 40) | ((uint64_t)ptr[3] << 32) |
           ((uint64_t)ptr[4] << 24) | ((uint64_t)ptr[5] << 16) |
           ((uint64_t)ptr[6] << 8)  | ((uint64_t)ptr[7]);
}

uint16_t bospectra_read_u16_le(const uint8_t* ptr) {
    if (!ptr) return 0;
    return ((uint16_t)ptr[1] << 8) | ((uint16_t)ptr[0]);
}

uint32_t bospectra_read_u32_le(const uint8_t* ptr) {
    if (!ptr) return 0;
    return ((uint32_t)ptr[3] << 24) | ((uint32_t)ptr[2] << 16) |
           ((uint32_t)ptr[1] << 8)  | ((uint32_t)ptr[0]);
}

uint64_t bospectra_read_u64_le(const uint8_t* ptr) {
    if (!ptr) return 0;
    return ((uint64_t)ptr[7] << 56) | ((uint64_t)ptr[6] << 48) |
           ((uint64_t)ptr[5] << 40) | ((uint64_t)ptr[4] << 32) |
           ((uint64_t)ptr[3] << 24) | ((uint64_t)ptr[2] << 16) |
           ((uint64_t)ptr[1] << 8)  | ((uint64_t)ptr[0]);
}

bool bospectra_container_check_bounds(uint64_t current_offset, uint64_t bytes_to_read, uint64_t total_file_size) {
    if (bytes_to_read == 0) return false;
    if (current_offset + bytes_to_read < current_offset) return false; // Integer overflow check
    if (current_offset + bytes_to_read > total_file_size) return false; // Out-of-bounds check
    return true;
}
