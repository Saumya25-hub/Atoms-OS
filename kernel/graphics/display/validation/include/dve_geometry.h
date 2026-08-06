#ifndef BOS_DVE_GEOMETRY_H
#define BOS_DVE_GEOMETRY_H

#include "kernel/graphics/display/validation/include/dve.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t stride;
    uint32_t bpp;
    uint32_t surface_pitch;
    uint32_t framebuffer_pitch;
    uint64_t actual_frame_size;
} dve_geometry_info_t;

typedef struct {
    dve_status_t status;
    uint32_t     expected_pitch;
    uint32_t     actual_pitch;
    uint32_t     expected_width;
    uint32_t     actual_width;
    uint32_t     expected_height;
    uint32_t     actual_height;
    uint64_t     expected_frame_size;
    uint64_t     actual_frame_size;
    bool         pitch_match;
    bool         stride_match;
    bool         resolution_match;
    char         failure_reason[128];
} dve_geometry_result_t;

dve_status_t dve_validate_geometry(const dve_geometry_info_t* info, dve_geometry_result_t* out_result);

#endif /* BOS_DVE_GEOMETRY_H */
