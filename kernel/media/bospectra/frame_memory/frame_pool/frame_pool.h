#ifndef FRAME_POOL_H
#define FRAME_POOL_H

#include "../include/bospectra_frame.h"

#define BOSPECTRA_FRAME_POOL_SIZE 8U
#define BOSPECTRA_DEFAULT_FRAME_WIDTH  1920U
#define BOSPECTRA_DEFAULT_FRAME_HEIGHT 1080U

void              bospectra_frame_pool_init(void);
void              bospectra_frame_pool_shutdown(void);
bospectra_error_t bospectra_frame_acquire(uint32_t width, uint32_t height, bospectra_pixel_format_t format, BOSFrame** out_frame);
bospectra_error_t bospectra_frame_release(BOSFrame* frame);
void              bospectra_frame_pool_get_counts(uint32_t* active, uint32_t* peak, uint32_t* capacity);

#endif // FRAME_POOL_H
