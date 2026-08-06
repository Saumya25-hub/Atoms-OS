#ifndef BOS_MODE_H
#define BOS_MODE_H

#include "kernel/graphics/display/include/bos_display.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t refresh_rate;     /* Hz (e.g. 24, 30, 50, 60, 75, 90, 120, 144, 165, 240) */
    uint32_t bpp;              /* 32bpp default */
    uint32_t pixel_clock_khz;
    uint32_t htotal;
    uint32_t vtotal;
    uint32_t flags;            /* Preferred, Native, Safe */
    char     name[32];
} bos_display_mode_t;

#define BOS_MODE_FLAG_PREFERRED             (1 << 0)
#define BOS_MODE_FLAG_NATIVE                (1 << 1)
#define BOS_MODE_FLAG_SAFE                  (1 << 2)

bool bos_mode_validate(const bos_display_mode_t* mode);
bool bos_mode_equal(const bos_display_mode_t* a, const bos_display_mode_t* b);

#endif /* BOS_MODE_H */
