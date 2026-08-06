#ifndef BOS_GPU_DRIVER_H
#define BOS_GPU_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/graphics/gpu/include/gpu_device.h"
#include "kernel/graphics/gpu/include/gpu_surface.h"

/* Status Codes */
typedef enum {
    BOS_GPU_OK = 0,
    BOS_GPU_ERR_GENERIC = -1,
    BOS_GPU_ERR_NOT_IMPLEMENTED = -2,
    BOS_GPU_ERR_INVALID_PARAM = -3,
    BOS_GPU_ERR_NO_MEMORY = -4,
    BOS_GPU_ERR_NOT_SUPPORTED = -5,
    BOS_GPU_ERR_DEVICE_BUSY = -6
} bos_gpu_status_t;

/* Driver Operations Interface Table */
typedef struct bos_gpu_driver_ops {
    bos_gpu_status_t (*init)(bos_gpu_device_t* dev);
    bos_gpu_status_t (*shutdown)(bos_gpu_device_t* dev);
    bos_gpu_status_t (*present)(bos_gpu_device_t* dev, bos_gpu_surface_t* surface);
    bos_gpu_status_t (*create_surface)(bos_gpu_device_t* dev, uint32_t width, uint32_t height, uint32_t format, bos_gpu_surface_t** out_surf);
    bos_gpu_status_t (*destroy_surface)(bos_gpu_device_t* dev, bos_gpu_surface_t* surface);
    bos_gpu_status_t (*map)(bos_gpu_device_t* dev, bos_gpu_surface_t* surface, void** out_ptr);
    bos_gpu_status_t (*unmap)(bos_gpu_device_t* dev, bos_gpu_surface_t* surface);
    bos_gpu_status_t (*fill_rect)(bos_gpu_device_t* dev, bos_gpu_surface_t* surf, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
    bos_gpu_status_t (*copy)(bos_gpu_device_t* dev, bos_gpu_surface_t* src, bos_gpu_surface_t* dst, uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h);
    bos_gpu_status_t (*stretch_copy)(bos_gpu_device_t* dev, bos_gpu_surface_t* src, bos_gpu_surface_t* dst, uint32_t sx, uint32_t sy, uint32_t sw, uint32_t sh, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh);
    bos_gpu_status_t (*wait_idle)(bos_gpu_device_t* dev);
    bos_gpu_status_t (*get_caps)(bos_gpu_device_t* dev, uint64_t* caps);
    bos_gpu_status_t (*set_cursor_position)(bos_gpu_device_t* dev, int32_t x, int32_t y);
    bos_gpu_status_t (*set_cursor_image)(bos_gpu_device_t* dev, const uint32_t* image, uint32_t w, uint32_t h, uint32_t hx, uint32_t hy);
    bos_gpu_status_t (*set_cursor_visibility)(bos_gpu_device_t* dev, bool visible);
} bos_gpu_driver_ops_t;

/* Driver Structure */
typedef struct bos_gpu_driver {
    char name[64];
    uint16_t vendor_id;
    uint16_t device_id;            /* 0xFFFF for wildcard match */
    bos_gpu_driver_ops_t ops;
    bool is_registered;
} bos_gpu_driver_t;

#endif /* BOS_GPU_DRIVER_H */
