#ifndef BITSTREAM_READER_H
#define BITSTREAM_READER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const uint8_t* buffer;
    size_t         size;
    size_t         bit_offset;
} BitstreamReader;

void     bitstream_init(BitstreamReader* bs, const uint8_t* buffer, size_t size);
uint32_t bitstream_read_bits(BitstreamReader* bs, uint32_t count);
uint32_t bitstream_peek_bits(const BitstreamReader* bs, uint32_t count);
void     bitstream_skip_bits(BitstreamReader* bs, uint32_t count);
void     bitstream_align_bits(BitstreamReader* bs);
bool     bitstream_has_more(const BitstreamReader* bs, uint32_t count);

#endif // BITSTREAM_READER_H
