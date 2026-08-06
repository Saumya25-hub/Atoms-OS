#ifndef BOS_DVE_PIXEL_FORMAT_H
#define BOS_DVE_PIXEL_FORMAT_H

#include "kernel/graphics/display/validation/include/dve.h"

typedef struct {
    dve_pixel_format_t format;
    uint32_t           bpp;
    uint32_t           red_mask;
    uint32_t           green_mask;
    uint32_t           blue_mask;
    uint32_t           alpha_mask;
    bool               has_alpha;
} dve_pixel_format_info_t;

typedef struct {
    dve_status_t status;
    bool         format_valid;
    bool         channel_ordering_valid;
    bool         alpha_valid;
    char         failure_reason[128];
} dve_pixel_format_result_t;

dve_status_t dve_validate_pixel_format(const dve_pixel_format_info_t* info, dve_pixel_format_result_t* out_result);

#endif /* BOS_DVE_PIXEL_FORMAT_H */
