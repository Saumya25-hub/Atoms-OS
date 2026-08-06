#ifndef BOS_DVE_FRAME_INTEGRITY_H
#define BOS_DVE_FRAME_INTEGRITY_H

#include "kernel/graphics/display/validation/include/dve.h"

typedef struct {
    const void* buffer_ptr;
    uint64_t    buffer_size;
    uint32_t    width;
    uint32_t    height;
    uint32_t    pitch;
    uint32_t    bpp;
} dve_frame_integrity_info_t;

typedef struct {
    dve_status_t status;
    uint32_t     crc32;
    uint64_t     fnv1a_hash;
    uint64_t     total_pixels;
    uint64_t     black_pixels;
    uint64_t     visible_pixels;
    float        black_pixel_percentage;
    float        visible_pixel_percentage;
    uint64_t     sum_red;
    uint64_t     sum_green;
    uint64_t     sum_blue;
    uint64_t     sum_alpha;
    uint32_t     entropy_score;
    bool         is_dead_frame;
    bool         is_frozen_frame;
    bool         scanline_shearing_detected;
    char         failure_reason[128];
} dve_frame_integrity_result_t;

dve_status_t dve_validate_frame_integrity(const dve_frame_integrity_info_t* info, dve_frame_integrity_result_t* out_result);

#endif /* BOS_DVE_FRAME_INTEGRITY_H */
