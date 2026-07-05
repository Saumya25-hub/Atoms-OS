#ifndef BOPAWN_H
#define BOPAWN_H

#include "kernel/gui/surface/surface.h"
#include <stdint.h>
#include <stdbool.h>

// Forward declaration of internal image structure
typedef struct BOSImage BOSImage;

// Public Engine API
void bopawn_init(void);

// Load an image from a path into an internal representation (with caching)
BOSImage* bopawn_load(const char* path);

// Manually destroy a loaded image (usually handled by cache ref-counting)
void bopawn_destroy(BOSImage* image);

// Pre-load an image into the cache
void bopawn_preload(const char* path);

// Reload an image from disk bypassing cache
BOSImage* bopawn_reload(const char* path);

// Get the actual BOSSurface containing decoded pixels (RGBA8888)
struct BOSSurface* bopawn_get_surface(BOSImage* image);

// Get image dimensions without necessarily returning surface
void bopawn_get_size(BOSImage* image, int* width, int* height);

struct BOSSurface* wallpaper_get(void);

#endif // BOPAWN_H
