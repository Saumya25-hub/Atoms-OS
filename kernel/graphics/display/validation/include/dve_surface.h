#ifndef BOS_DVE_SURFACE_H
#define BOS_DVE_SURFACE_H

#include "kernel/graphics/display/validation/include/dve.h"

typedef struct {
    uint32_t           surface_id;
    uint32_t           width;
    uint32_t           height;
    uint32_t           pitch;
    dve_pixel_format_t format;
    uint32_t           alignment;
    bool               is_active;
    bool               handle_valid;
} dve_surface_info_t;

typedef struct {
    dve_status_t status;
    bool         dimensions_valid;
    bool         pitch_valid;
    bool         format_supported;
    bool         alignment_valid;
    bool         lifetime_valid;
    char         failure_reason[128];
} dve_surface_result_t;

dve_status_t dve_validate_surface(const dve_surface_info_t* info, dve_surface_result_t* out_result);

#endif /* BOS_DVE_SURFACE_H */
