#include "kernel/media/bopawn/bopawn.h"
#include "kernel/media/bopawn/formats/image_format.h"
#include <stddef.h>


// Forward declarations for cache and loader
extern BOSImage* image_cache_get(const char* path);
extern void image_cache_put(BOSImage* image);
extern uint8_t* image_loader_read(const char* path, uint32_t* out_size);
extern void image_loader_free(uint8_t* buffer);

void bopawn_init(void) {
    // Initialize cache and wallpaper engine if needed
}

BOPAWN_Format bopawn_detect_format(const uint8_t* buffer, uint32_t size) {
    if (!buffer || size < 8) return BOPAWN_FORMAT_UNKNOWN;

    // PNG: 89 50 4E 47 0D 0A 1A 0A
    if (buffer[0] == 0x89 && buffer[1] == 0x50 && buffer[2] == 0x4E && buffer[3] == 0x47) {
        return BOPAWN_FORMAT_PNG;
    }
    // BMP: 'B' 'M'
    if (buffer[0] == 'B' && buffer[1] == 'M') {
        return BOPAWN_FORMAT_BMP;
    }
    // ICO: 00 00 01 00
    if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x01 && buffer[3] == 0x00) {
        return BOPAWN_FORMAT_ICO;
    }
    
    // Assume RAW if nothing else matches (as RAW lacks magic bytes usually)
    return BOPAWN_FORMAT_RAW;
}

BOSImage* bopawn_load(const char* path) {
    if (!path) return NULL;
    
    // Check cache first
    BOSImage* cached = image_cache_get(path);
    if (cached) {
        return cached;
    }
    
    // Not in cache, read from disk
    uint32_t file_size = 0;
    uint8_t* buffer = image_loader_read(path, &file_size);
    if (!buffer) return NULL;
    
    BOPAWN_Format format = bopawn_detect_format(buffer, file_size);
    struct BOSSurface* surface = NULL;
    
    switch (format) {
        case BOPAWN_FORMAT_PNG: surface = png_decode(buffer, file_size); break;
        case BOPAWN_FORMAT_BMP: surface = bmp_decode(buffer, file_size); break;
        case BOPAWN_FORMAT_ICO: surface = ico_decode(buffer, file_size); break;
        case BOPAWN_FORMAT_RAW: surface = raw_decode(buffer, file_size); break;
        default: break;
    }
    
    image_loader_free(buffer);
    
    if (!surface) return NULL;
    
    // Build image object
    // Assuming a simple malloc for BOSImage is available
    extern void* kmalloc(uint32_t size);
    BOSImage* image = (BOSImage*)kmalloc(sizeof(BOSImage));
    if (!image) {
        surface_destroy(surface);
        return NULL;
    }
    
    image->format = format;
    image->surface = surface;
    image->width = surface->width;
    image->height = surface->height;
    image->ref_count = 0; // cache_put increments to 1
    
    size_t i = 0;
    while (path[i] && i < 255) {
        image->filepath[i] = path[i];
        i++;
    }
    image->filepath[i] = '\0';
    
    image_cache_put(image);
    
    return image;
}

void bopawn_destroy(BOSImage* image) {
    extern void image_cache_release(BOSImage* img);
    if (image) image_cache_release(image);
}

void bopawn_preload(const char* path) {
    bopawn_load(path);
}

BOSImage* bopawn_reload(const char* path) {
    extern void image_cache_remove(const char* p);
    image_cache_remove(path);
    return bopawn_load(path);
}

struct BOSSurface* bopawn_get_surface(BOSImage* image) {
    if (!image) return NULL;
    return image->surface;
}

void bopawn_get_size(BOSImage* image, int* width, int* height) {
    if (image) {
        if (width) *width = image->width;
        if (height) *height = image->height;
    }
}
