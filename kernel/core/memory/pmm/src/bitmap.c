#include "kernel/core/memory/pmm/include/bitmap.h"

void bitmap_set(uint8_t* bitmap, uint64_t bit) {
    bitmap[bit / 8] |= (1 << (bit % 8));
}

void bitmap_clear(uint8_t* bitmap, uint64_t bit) {
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

bool bitmap_test(uint8_t* bitmap, uint64_t bit) {
    return (bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}
