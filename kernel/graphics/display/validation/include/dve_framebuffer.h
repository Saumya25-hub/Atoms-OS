#ifndef BOS_DVE_FRAMEBUFFER_H
#define BOS_DVE_FRAMEBUFFER_H

#include "kernel/graphics/display/validation/include/dve.h"

typedef struct {
    uint64_t phys_base;
    void*    virt_base;
    uint64_t total_size_bytes;
    uint32_t alignment_bytes;
    bool     is_phys_mapped;
    bool     is_virt_mapped;
    bool     overflow_detected;
    bool     bounds_valid;
} dve_framebuffer_info_t;

typedef struct {
    dve_status_t status;
    bool         base_address_valid;
    bool         size_sufficient;
    bool         memory_aligned;
    bool         page_aligned;
    bool         physical_mapped;
    bool         virtual_mapped;
    bool         overflow_pass;
    bool         bounds_check_pass;
    char         failure_reason[128];
} dve_framebuffer_result_t;

dve_status_t dve_validate_framebuffer(const dve_framebuffer_info_t* info, dve_framebuffer_result_t* out_result);

#endif /* BOS_DVE_FRAMEBUFFER_H */
