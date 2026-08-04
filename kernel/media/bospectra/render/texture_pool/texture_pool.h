#ifndef TEXTURE_POOL_H
#define TEXTURE_POOL_H

#include "kernel/ui/boimage/boimage.h"
#include "../../include/bospectra_errors.h"

#define BOSPECTRA_TEXTURE_POOL_SIZE 4U

void              bospectra_texture_pool_init(void);
void              bospectra_texture_pool_shutdown(void);
bospectra_error_t bospectra_texture_acquire(uint32_t width, uint32_t height, BOTexture** out_texture);
bospectra_error_t bospectra_texture_release(BOTexture* texture);
void              bospectra_texture_pool_get_counts(uint32_t* active, uint32_t* peak, uint32_t* capacity);

#endif // TEXTURE_POOL_H
