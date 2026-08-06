#ifndef BOS_GPU_H
#define BOS_GPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "kernel/graphics/gpu/include/gpu_caps.h"
#include "kernel/graphics/gpu/include/gpu_surface.h"
#include "kernel/graphics/gpu/include/gpu_device.h"
#include "kernel/graphics/gpu/include/gpu_driver.h"

#define MAX_GPU_DEVICES 16
#define MAX_GPU_DRIVERS 16

/* Primary GPU Subsystem Public HAL API */
bos_gpu_status_t bos_gpu_init(void);
bos_gpu_status_t bos_gpu_shutdown(void);
uint32_t         bos_gpu_enumerate(void);
bos_gpu_status_t bos_gpu_register_driver(bos_gpu_driver_t* driver);
bos_gpu_device_t* bos_gpu_get_primary(void);
bos_gpu_status_t bos_gpu_get_caps(bos_gpu_device_t* dev, uint64_t* out_caps);
bos_gpu_status_t bos_gpu_present(bos_gpu_surface_t* surface);

/* Additional Subsystem Helpers */
uint32_t         bos_gpu_get_device_count(void);
bos_gpu_device_t* bos_gpu_get_device(uint32_t index);
bos_gpu_status_t bos_gpu_set_primary(bos_gpu_device_t* dev);

/* Diagnostics & Verification */
void             bos_gpu_print_diagnostics(void);
void             bos_gpu_run_tests(void);

#endif /* BOS_GPU_H */
