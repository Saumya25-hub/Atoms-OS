#ifndef BOS_DVE_PRESENTATION_H
#define BOS_DVE_PRESENTATION_H

#include "kernel/graphics/display/validation/include/dve.h"

typedef struct {
    uint32_t src_width;
    uint32_t src_height;
    uint32_t src_pitch;
    uint32_t dst_width;
    uint32_t dst_height;
    uint32_t dst_pitch;
    uint64_t copy_size_bytes;
    bool     page_flip_supported;
    bool     buffer_swap_active;
    bool     dma_copy_active;
    bool     hw_present_active;
} dve_presentation_info_t;

typedef struct {
    dve_status_t status;
    uint64_t     expected_copy_size;
    uint64_t     actual_copy_size;
    uint32_t     expected_src_pitch;
    uint32_t     actual_src_pitch;
    uint32_t     expected_dst_pitch;
    uint32_t     actual_dst_pitch;
    bool         present_call_pass;
    bool         page_flip_pass;
    bool         buffer_swap_pass;
    bool         copy_size_pass;
    bool         pitch_match_pass;
    bool         dma_copy_pass;
    bool         hw_present_pass;
    char         failure_reason[128];
} dve_presentation_result_t;

dve_status_t dve_validate_presentation(const dve_presentation_info_t* info, dve_presentation_result_t* out_result);

#endif /* BOS_DVE_PRESENTATION_H */
