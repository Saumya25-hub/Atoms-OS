#include "bitstream_reader.h"
#include "kernel/core/lib/include/string.h"

void bitstream_init(BitstreamReader* bs, const uint8_t* buffer, size_t size) {
    if (!bs) return;
    bs->buffer = buffer;
    bs->size = size;
    bs->bit_offset = 0;
}

bool bitstream_has_more(const BitstreamReader* bs, uint32_t count) {
    if (!bs || !bs->buffer) return false;
    return (bs->bit_offset + count) <= (bs->size * 8);
}

uint32_t bitstream_read_bits(BitstreamReader* bs, uint32_t count) {
    if (!bs || count == 0 || count > 32 || !bitstream_has_more(bs, count)) return 0;

    uint32_t result = 0;
    for (uint32_t i = 0; i < count; i++) {
        size_t byte_idx = bs->bit_offset / 8;
        size_t bit_idx = 7 - (bs->bit_offset % 8);
        uint32_t bit = (bs->buffer[byte_idx] >> bit_idx) & 1;
        result = (result << 1) | bit;
        bs->bit_offset++;
    }
    return result;
}

uint32_t bitstream_peek_bits(const BitstreamReader* bs, uint32_t count) {
    if (!bs || count == 0 || count > 32 || !bitstream_has_more(bs, count)) return 0;

    BitstreamReader temp = *bs;
    return bitstream_read_bits(&temp, count);
}

void bitstream_skip_bits(BitstreamReader* bs, uint32_t count) {
    if (!bs) return;
    if (bs->bit_offset + count <= bs->size * 8) {
        bs->bit_offset += count;
    } else {
        bs->bit_offset = bs->size * 8;
    }
}

void bitstream_align_bits(BitstreamReader* bs) {
    if (!bs) return;
    size_t rem = bs->bit_offset % 8;
    if (rem != 0) {
        bs->bit_offset += (8 - rem);
    }
}
