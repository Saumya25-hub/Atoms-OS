#ifndef BOS_DVE_DISPLAY_MODE_H
#define BOS_DVE_DISPLAY_MODE_H

#include "kernel/graphics/display/validation/include/dve.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t refresh_rate;
    uint32_t pixel_clock_khz;
    uint32_t htotal;
    uint32_t vtotal;
    bool     scanout_active;
    bool     monitor_connected;
    bool     edid_valid;
} dve_display_mode_info_t;

typedef struct {
    dve_status_t status;
    bool         resolution_valid;
    bool         refresh_rate_valid;
    bool         timing_valid;
    bool         scanout_valid;
    bool         mode_supported;
    bool         monitor_connected;
    char         failure_reason[128];
} dve_display_mode_result_t;

dve_status_t dve_validate_display_mode(const dve_display_mode_info_t* info, dve_display_mode_result_t* out_result);

#endif /* BOS_DVE_DISPLAY_MODE_H */
