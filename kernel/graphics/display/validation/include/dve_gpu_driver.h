#ifndef BOS_DVE_GPU_DRIVER_H
#define BOS_DVE_GPU_DRIVER_H

#include "kernel/graphics/display/validation/include/dve.h"

typedef struct {
    const char* driver_name;
    uint16_t    vendor_id;
    uint16_t    device_id;
    uint64_t    bar0_base;
    uint64_t    bar0_size;
    uint64_t    bar1_vram_base;
    uint64_t    bar1_vram_size;
    uint64_t    capabilities;
    bool        driver_loaded;
    bool        driver_selected;
    bool        mmio_mapped;
} dve_gpu_driver_info_t;

typedef struct {
    dve_status_t status;
    bool         loaded_pass;
    bool         selection_pass;
    bool         vendor_device_valid;
    bool         bar_mapping_valid;
    bool         mmio_mapping_valid;
    bool         capabilities_valid;
    char         failure_reason[128];
} dve_gpu_driver_result_t;

dve_status_t dve_validate_gpu_driver(const dve_gpu_driver_info_t* info, dve_gpu_driver_result_t* out_result);

#endif /* BOS_DVE_GPU_DRIVER_H */
