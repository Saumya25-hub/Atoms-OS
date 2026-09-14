#ifndef BOS_UI_PNG_HPP
#define BOS_UI_PNG_HPP

#include <stdint.h>
#include <stddef.h>
#include "types.hpp"
#include "resource.hpp"

namespace bos {

// ============================================================================
// ATOMS OS Native PNG Decoder Engine
// ============================================================================
class PngDecoder {
public:
    // Decodes in-memory PNG byte stream into 32-bpp ARGB Bitmap
    static Result decode(const uint8_t* buffer, size_t size, Bitmap& out_bitmap);

    // Loads and decodes PNG directly from the filesystem (BOFS / FAT32)
    static Result decode_file(const char* path, Bitmap& out_bitmap);
};

} // namespace bos

#endif // BOS_UI_PNG_HPP
