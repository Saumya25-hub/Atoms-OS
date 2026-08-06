#ifndef BOS_GPU_SURFACE_H
#define BOS_GPU_SURFACE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Pixel Formats */
typedef enum {
    BOS_GPU_FORMAT_UNKNOWN = 0,
    BOS_GPU_FORMAT_RGBA8888,
    BOS_GPU_FORMAT_BGRA8888,
    BOS_GPU_FORMAT_RGB565,
    BOS_GPU_FORMAT_XRGB8888,
    BOS_GPU_FORMAT_MAX
} bos_gpu_format_t;

/* Surface Flags */
#define BOS_GPU_SURFACE_FLAG_NONE       0x00
#define BOS_GPU_SURFACE_FLAG_HARDWARE   0x01  /* Backed by VRAM / HW memory */
#define BOS_GPU_SURFACE_FLAG_CPU_MAPPED 0x02  /* Mapped into CPU address space */
#define BOS_GPU_SURFACE_FLAG_PRIMARY    0x04  /* Primary Scanout Surface */

/* GPU Surface Handle / Descriptor */
typedef struct bos_gpu_surface {
    uint32_t handle;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;              /* Bytes per row */
    uint32_t bpp;                /* Bits per pixel */
    bos_gpu_format_t format;
    uint32_t flags;
    uint64_t phys_addr;          /* Physical address in VRAM/RAM */
    void* virt_addr;             /* Mapped CPU virtual pointer */
    size_t size;                 /* Allocation size in bytes */
    void* private_data;          /* Driver private metadata */
} bos_gpu_surface_t;

#endif /* BOS_GPU_SURFACE_H */
