#include "kernel/graphics/gpu/include/gpu.h"
#include "kernel/graphics/gpu/memory/gpu_memory.h"
#include "kernel/graphics/gpu/debug/gpu_debug.h"

extern void display_print(const char* str);

static bos_gpu_status_t swrender_init(bos_gpu_device_t* dev) {
    (void)dev;
    gpu_log_info("Software Renderer Fallback Driver Initialized.");
    return BOS_GPU_OK;
}

static bos_gpu_status_t swrender_shutdown(bos_gpu_device_t* dev) {
    (void)dev;
    gpu_log_info("Software Renderer Fallback Driver Shutdown.");
    return BOS_GPU_OK;
}

static bos_gpu_status_t swrender_present(bos_gpu_device_t* dev, bos_gpu_surface_t* surface) {
    (void)dev;
    if (!surface || !surface->virt_addr) return BOS_GPU_ERR_INVALID_PARAM;
    /* Soft sync/present: In Phase 1 software renderer, memory surface scanout is active */
    return BOS_GPU_OK;
}

static bos_gpu_status_t swrender_create_surface(bos_gpu_device_t* dev, uint32_t w, uint32_t h, uint32_t format, bos_gpu_surface_t** out_surf) {
    (void)dev;
    if (!out_surf || w == 0 || h == 0) return BOS_GPU_ERR_INVALID_PARAM;

    bos_gpu_surface_t* surf = (bos_gpu_surface_t*)gpu_mem_alloc(sizeof(bos_gpu_surface_t));
    if (!surf) return BOS_GPU_ERR_NO_MEMORY;

    uint32_t bpp = 32;
    uint32_t pitch = w * 4;
    size_t total_size = (size_t)pitch * h;
    void* ptr = gpu_mem_alloc(total_size);
    if (!ptr) {
        gpu_mem_free(surf);
        return BOS_GPU_ERR_NO_MEMORY;
    }

    surf->handle = 1;
    surf->width = w;
    surf->height = h;
    surf->pitch = pitch;
    surf->bpp = bpp;
    surf->format = (bos_gpu_format_t)format;
    surf->flags = BOS_GPU_SURFACE_FLAG_CPU_MAPPED;
    surf->phys_addr = (uint64_t)(uintptr_t)ptr;
    surf->virt_addr = ptr;
    surf->size = total_size;
    surf->private_data = NULL;

    *out_surf = surf;
    return BOS_GPU_OK;
}

static bos_gpu_status_t swrender_destroy_surface(bos_gpu_device_t* dev, bos_gpu_surface_t* surface) {
    (void)dev;
    if (!surface) return BOS_GPU_ERR_INVALID_PARAM;
    if (surface->virt_addr) {
        gpu_mem_free(surface->virt_addr);
    }
    gpu_mem_free(surface);
    return BOS_GPU_OK;
}

static bos_gpu_status_t swrender_map(bos_gpu_device_t* dev, bos_gpu_surface_t* surface, void** out_ptr) {
    (void)dev;
    if (!surface || !out_ptr) return BOS_GPU_ERR_INVALID_PARAM;
    *out_ptr = surface->virt_addr;
    return BOS_GPU_OK;
}

static bos_gpu_status_t swrender_unmap(bos_gpu_device_t* dev, bos_gpu_surface_t* surface) {
    (void)dev; (void)surface;
    return BOS_GPU_OK;
}

static bos_gpu_status_t swrender_fill_rect(bos_gpu_device_t* dev, bos_gpu_surface_t* surf, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    (void)dev;
    if (!surf || !surf->virt_addr) return BOS_GPU_ERR_INVALID_PARAM;

    if (x >= surf->width || y >= surf->height) return BOS_GPU_OK;
    if (x + w > surf->width) w = surf->width - x;
    if (y + h > surf->height) h = surf->height - y;

    uint32_t* pixels = (uint32_t*)surf->virt_addr;
    uint32_t stride = surf->pitch / 4;

    for (uint32_t row = 0; row < h; row++) {
        uint32_t* line = pixels + ((y + row) * stride) + x;
        for (uint32_t col = 0; col < w; col++) {
            line[col] = color;
        }
    }

    return BOS_GPU_OK;
}

static bos_gpu_status_t swrender_copy(bos_gpu_device_t* dev, bos_gpu_surface_t* src, bos_gpu_surface_t* dst, uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h) {
    (void)dev;
    if (!src || !dst || !src->virt_addr || !dst->virt_addr) return BOS_GPU_ERR_INVALID_PARAM;

    if (sx >= src->width || sy >= src->height || dx >= dst->width || dy >= dst->height) return BOS_GPU_OK;
    if (sx + w > src->width) w = src->width - sx;
    if (sy + h > src->height) h = src->height - sy;
    if (dx + w > dst->width) w = dst->width - dx;
    if (dy + h > dst->height) h = dst->height - dy;

    const uint32_t* src_pixels = (const uint32_t*)src->virt_addr;
    uint32_t* dst_pixels = (uint32_t*)dst->virt_addr;

    uint32_t src_stride = src->pitch / 4;
    uint32_t dst_stride = dst->pitch / 4;

    for (uint32_t row = 0; row < h; row++) {
        const uint32_t* src_line = src_pixels + ((sy + row) * src_stride) + sx;
        uint32_t* dst_line = dst_pixels + ((dy + row) * dst_stride) + dx;
        for (uint32_t col = 0; col < w; col++) {
            dst_line[col] = src_line[col];
        }
    }

    return BOS_GPU_OK;
}

static bos_gpu_status_t swrender_stretch_copy(bos_gpu_device_t* dev, bos_gpu_surface_t* src, bos_gpu_surface_t* dst, uint32_t sx, uint32_t sy, uint32_t sw, uint32_t sh, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh) {
    (void)dev;
    if (!src || !dst || !src->virt_addr || !dst->virt_addr || sw == 0 || sh == 0 || dw == 0 || dh == 0) {
        return BOS_GPU_ERR_INVALID_PARAM;
    }

    const uint32_t* src_pixels = (const uint32_t*)src->virt_addr;
    uint32_t* dst_pixels = (uint32_t*)dst->virt_addr;

    uint32_t src_stride = src->pitch / 4;
    uint32_t dst_stride = dst->pitch / 4;

    for (uint32_t row = 0; row < dh; row++) {
        uint32_t src_y = sy + ((row * sh) / dh);
        if (src_y >= src->height) break;
        uint32_t dst_y = dy + row;
        if (dst_y >= dst->height) break;

        const uint32_t* src_line = src_pixels + (src_y * src_stride);
        uint32_t* dst_line = dst_pixels + (dst_y * dst_stride);

        for (uint32_t col = 0; col < dw; col++) {
            uint32_t src_x = sx + ((col * sw) / dw);
            if (src_x >= src->width) break;
            uint32_t dst_x = dx + col;
            if (dst_x >= dst->width) break;

            dst_line[dst_x] = src_line[src_x];
        }
    }

    return BOS_GPU_OK;
}

static bos_gpu_status_t swrender_wait_idle(bos_gpu_device_t* dev) {
    (void)dev;
    /* Software execution is synchronous */
    return BOS_GPU_OK;
}

static bos_gpu_status_t swrender_get_caps(bos_gpu_device_t* dev, uint64_t* caps) {
    (void)dev;
    if (!caps) return BOS_GPU_ERR_INVALID_PARAM;
    *caps = BOS_GPU_CAP_FRAMEBUFFER | BOS_GPU_CAP_DOUBLE_BUFFER;
    return BOS_GPU_OK;
}

static bos_gpu_driver_t g_swrender_driver = {
    .name = "BOS Software Renderer Fallback Driver",
    .vendor_id = BOS_GPU_VENDOR_SOFTWARE,
    .device_id = 0x0001,
    .is_registered = false,
    .ops = {
        .init = swrender_init,
        .shutdown = swrender_shutdown,
        .present = swrender_present,
        .create_surface = swrender_create_surface,
        .destroy_surface = swrender_destroy_surface,
        .map = swrender_map,
        .unmap = swrender_unmap,
        .fill_rect = swrender_fill_rect,
        .copy = swrender_copy,
        .stretch_copy = swrender_stretch_copy,
        .wait_idle = swrender_wait_idle,
        .get_caps = swrender_get_caps
    }
};

bos_gpu_status_t gpu_driver_swrender_register(void) {
    return bos_gpu_register_driver(&g_swrender_driver);
}

bos_gpu_driver_t* gpu_driver_swrender_get(void) {
    return &g_swrender_driver;
}
