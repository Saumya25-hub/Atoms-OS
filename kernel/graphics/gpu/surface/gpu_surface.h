#ifndef BOS_GPU_SURFACE_HAL_H
#define BOS_GPU_SURFACE_HAL_H

#include "kernel/graphics/gpu/include/gpu.h"

bos_gpu_status_t gpu_surface_create(bos_gpu_device_t* dev, uint32_t width, uint32_t height, bos_gpu_format_t format, bos_gpu_surface_t** out_surf);
bos_gpu_status_t gpu_surface_destroy(bos_gpu_device_t* dev, bos_gpu_surface_t* surface);
bos_gpu_status_t gpu_surface_map(bos_gpu_device_t* dev, bos_gpu_surface_t* surface, void** out_ptr);
bos_gpu_status_t gpu_surface_unmap(bos_gpu_device_t* dev, bos_gpu_surface_t* surface);

#endif /* BOS_GPU_SURFACE_HAL_H */
