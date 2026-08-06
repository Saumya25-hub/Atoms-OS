#include "kernel/graphics/gpu/surface/gpu_surface.h"
#include "kernel/graphics/gpu/memory/gpu_memory.h"
#include "kernel/graphics/gpu/debug/gpu_debug.h"

static uint32_t g_surface_handle_counter = 1000;

static uint32_t get_bpp_for_format(bos_gpu_format_t format) {
    switch (format) {
        case BOS_GPU_FORMAT_RGBA8888:
        case BOS_GPU_FORMAT_BGRA8888:
        case BOS_GPU_FORMAT_XRGB8888:
            return 32;
        case BOS_GPU_FORMAT_RGB565:
            return 16;
        default:
            return 32;
    }
}

bos_gpu_status_t gpu_surface_create(bos_gpu_device_t* dev, uint32_t width, uint32_t height, bos_gpu_format_t format, bos_gpu_surface_t** out_surf) {
    if (!out_surf || width == 0 || height == 0) {
        return BOS_GPU_ERR_INVALID_PARAM;
    }

    if (dev && dev->ops && dev->ops->create_surface) {
        return dev->ops->create_surface(dev, width, height, (uint32_t)format, out_surf);
    }

    /* System Memory Fallback Surface */
    bos_gpu_surface_t* surf = (bos_gpu_surface_t*)gpu_mem_alloc(sizeof(bos_gpu_surface_t));
    if (!surf) return BOS_GPU_ERR_NO_MEMORY;

    uint32_t bpp = get_bpp_for_format(format);
    uint32_t pitch = width * (bpp / 8);
    size_t total_size = (size_t)pitch * height;

    void* buffer = gpu_mem_alloc(total_size);
    if (!buffer) {
        gpu_mem_free(surf);
        return BOS_GPU_ERR_NO_MEMORY;
    }

    surf->handle = g_surface_handle_counter++;
    surf->width = width;
    surf->height = height;
    surf->pitch = pitch;
    surf->bpp = bpp;
    surf->format = format;
    surf->flags = BOS_GPU_SURFACE_FLAG_CPU_MAPPED;
    surf->phys_addr = (uint64_t)(uintptr_t)buffer;
    surf->virt_addr = buffer;
    surf->size = total_size;
    surf->private_data = NULL;

    *out_surf = surf;
    return BOS_GPU_OK;
}

bos_gpu_status_t gpu_surface_destroy(bos_gpu_device_t* dev, bos_gpu_surface_t* surface) {
    if (!surface) return BOS_GPU_ERR_INVALID_PARAM;

    if (dev && dev->ops && dev->ops->destroy_surface) {
        return dev->ops->destroy_surface(dev, surface);
    }

    if (surface->virt_addr) {
        gpu_mem_free(surface->virt_addr);
    }
    gpu_mem_free(surface);
    return BOS_GPU_OK;
}

bos_gpu_status_t gpu_surface_map(bos_gpu_device_t* dev, bos_gpu_surface_t* surface, void** out_ptr) {
    if (!surface || !out_ptr) return BOS_GPU_ERR_INVALID_PARAM;

    if (dev && dev->ops && dev->ops->map) {
        return dev->ops->map(dev, surface, out_ptr);
    }

    *out_ptr = surface->virt_addr;
    return BOS_GPU_OK;
}

bos_gpu_status_t gpu_surface_unmap(bos_gpu_device_t* dev, bos_gpu_surface_t* surface) {
    if (!surface) return BOS_GPU_ERR_INVALID_PARAM;

    if (dev && dev->ops && dev->ops->unmap) {
        return dev->ops->unmap(dev, surface);
    }

    return BOS_GPU_OK;
}
