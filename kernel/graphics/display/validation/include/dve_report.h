#ifndef BOS_DVE_REPORT_H
#define BOS_DVE_REPORT_H

#include "kernel/graphics/display/validation/include/dve.h"
#include "kernel/graphics/display/validation/include/dve_framebuffer.h"
#include "kernel/graphics/display/validation/include/dve_geometry.h"
#include "kernel/graphics/display/validation/include/dve_pixel_format.h"
#include "kernel/graphics/display/validation/include/dve_surface.h"
#include "kernel/graphics/display/validation/include/dve_gpu_driver.h"
#include "kernel/graphics/display/validation/include/dve_display_mode.h"
#include "kernel/graphics/display/validation/include/dve_presentation.h"
#include "kernel/graphics/display/validation/include/dve_memory_safety.h"
#include "kernel/graphics/display/validation/include/dve_frame_integrity.h"

typedef struct {
    dve_framebuffer_result_t   framebuffer_res;
    dve_geometry_result_t      geometry_res;
    dve_pixel_format_result_t  format_res;
    dve_surface_result_t       surface_res;
    dve_gpu_driver_result_t    driver_res;
    dve_display_mode_result_t  mode_res;
    dve_presentation_result_t  presentation_res;
    dve_memory_safety_result_t memory_res;
    dve_frame_integrity_result_t integrity_res;
    
    char                       root_cause_subsystem[128];
    dve_confidence_t           confidence;
    bool                       overall_pass;
} dve_diagnostic_report_t;

void dve_report_init(dve_diagnostic_report_t* report);
void dve_report_compile(dve_diagnostic_report_t* report);
void dve_report_print(const dve_diagnostic_report_t* report);

#endif /* BOS_DVE_REPORT_H */
