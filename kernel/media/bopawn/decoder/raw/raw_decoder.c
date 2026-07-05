#include "kernel/media/bopawn/formats/image_format.h"
#include "kernel/core/memory/heap/include/heap.h"

// Assuming raw files are simply 1920x1080 BGRA data in ATOMS OS
#define RAW_DEFAULT_WIDTH 1920
#define RAW_DEFAULT_HEIGHT 1080

struct BOSSurface* raw_decode(const uint8_t* buffer, uint32_t size) {
    if (!buffer || size == 0) return NULL;
    
    // We assume standard 1080p raw background files for now
    // A robust engine would pass width/height or parse a custom header.
    // Since existing .raw files are exact framebuffer dumps:
    int w = RAW_DEFAULT_WIDTH;
    int h = RAW_DEFAULT_HEIGHT;
    
    if (size < w * h * 4) {
        // Not enough data
        return NULL;
    }
    
    uint32_t* pixels = (uint32_t*)kmalloc(w * h * 4);
    if (!pixels) return NULL;
    
    extern void* memcpy(void*, const void*, size_t);
    memcpy(pixels, buffer, w * h * 4);
    
    struct BOSSurface* surface = surface_convert_rgba(pixels, w, h);
    kfree(pixels); // surface_convert copies it
    
    return surface;
}
