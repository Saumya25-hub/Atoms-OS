#ifndef BOS_DVE_MEMORY_SAFETY_H
#define BOS_DVE_MEMORY_SAFETY_H

#include "kernel/graphics/display/validation/include/dve.h"

typedef struct {
    void*    buffer_ptr;
    uint64_t buffer_size;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    uint32_t access_x;
    uint32_t access_y;
    uint32_t access_width;
    uint32_t access_height;
} dve_memory_safety_info_t;

typedef struct {
    dve_status_t status;
    bool         no_overrun;
    bool         no_underrun;
    bool         pointer_valid;
    bool         pitch_valid;
    bool         surface_not_null;
    bool         bounds_valid;
    bool         alignment_valid;
    char         failure_reason[128];
} dve_memory_safety_result_t;

dve_status_t dve_validate_memory_safety(const dve_memory_safety_info_t* info, dve_memory_safety_result_t* out_result);

#endif /* BOS_DVE_MEMORY_SAFETY_H */
