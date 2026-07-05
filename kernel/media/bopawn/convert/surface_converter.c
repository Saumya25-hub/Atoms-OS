#include "kernel/media/bopawn/formats/image_format.h"
#include "kernel/core/memory/heap/include/heap.h"

struct BOSSurface* surface_convert_rgba(uint32_t* pixels, int width, int height) {
    if (!pixels || width <= 0 || height <= 0) return NULL;
    
    struct BOSSurface* surface = surface_create(width, height);
    if (!surface) return NULL;
    
    // Copy pixels to surface framebuffer
    // Assuming pixels are already in target OS format (e.g. BGRA 8888 for ATOMS)
    // If conversion was needed, it would happen here.
    uint32_t* dest = surface->framebuffer;
    uint32_t* src = pixels;
    int count = width * height;
    
    for (int i = 0; i < count; i++) {
        dest[i] = src[i];
    }
    
    return surface;
}
