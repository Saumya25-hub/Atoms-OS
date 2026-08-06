#ifndef BOS_DVE_H
#define BOS_DVE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    DVE_STATUS_PASS = 0,
    DVE_STATUS_FAIL = -1,
    DVE_STATUS_WARN = 1
} dve_status_t;

typedef enum {
    DVE_CONFIDENCE_NONE = 0,
    DVE_CONFIDENCE_LOW = 1,
    DVE_CONFIDENCE_MEDIUM = 2,
    DVE_CONFIDENCE_HIGH = 3
} dve_confidence_t;

typedef enum {
    DVE_PIXEL_FORMAT_UNKNOWN = 0,
    DVE_PIXEL_FORMAT_ARGB8888,
    DVE_PIXEL_FORMAT_XRGB8888,
    DVE_PIXEL_FORMAT_RGB565,
    DVE_PIXEL_FORMAT_RGB888,
    DVE_PIXEL_FORMAT_BGRA8888
} dve_pixel_format_t;

#endif /* BOS_DVE_H */
