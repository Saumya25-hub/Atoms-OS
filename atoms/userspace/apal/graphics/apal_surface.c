/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Graphics Surface Implementation
 */

#include "apal_surface.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"
#include <string.h>

#define SYS_GUI_CREATE_WINDOW   16ULL
#define SYS_GUI_DESTROY_WINDOW  17ULL
#define SYS_GUI_MAP_SURFACE     20ULL
#define SYS_GUI_INVALIDATE      21ULL

apal_status_t apal_surface_create(uint32_t width, uint32_t height, const char *title, apal_surface_t *out_surface) {
    if (!out_surface || width == 0 || height == 0) return APAL_ERR_INVALID_PARAM;

    int64_t win_id = __syscall6(SYS_GUI_CREATE_WINDOW, 100, 100, width, height, 0, (int64_t)title);
    if (win_id <= 0) {
        /* Fallback if running headless or outside compositor: allocate local 32-bpp buffer */
        size_t buf_sz = (size_t)width * height * 4;
        void *local_pixels = atoms_sys_mmap(NULL, buf_sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (!local_pixels || local_pixels == (void *)-1) {
            return APAL_ERR_NO_MEMORY;
        }
        out_surface->handle = 1;
        out_surface->window_id = 0;
        out_surface->width = width;
        out_surface->height = height;
        out_surface->stride_bytes = width * 4;
        out_surface->pixel_buffer = (uint32_t *)local_pixels;
        return APAL_OK;
    }

    uint64_t surface_ptr = 0;
    uint32_t stride = 0;
    int64_t map_ret = __syscall3(SYS_GUI_MAP_SURFACE, win_id, (int64_t)&surface_ptr, (int64_t)&stride);
    if (map_ret != 0 || surface_ptr == 0) {
        __syscall1(SYS_GUI_DESTROY_WINDOW, win_id);
        return APAL_ERR_INTERNAL;
    }

    out_surface->handle = (apal_surface_handle_t)win_id;
    out_surface->window_id = (uint32_t)win_id;
    out_surface->width = width;
    out_surface->height = height;
    out_surface->stride_bytes = (stride > 0) ? stride : (width * 4);
    out_surface->pixel_buffer = (uint32_t *)surface_ptr;
    return APAL_OK;
}

apal_status_t apal_surface_present(apal_surface_t *surface, int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!surface || !surface->pixel_buffer) return APAL_ERR_INVALID_PARAM;
    if (surface->window_id > 0) {
        __syscall5(SYS_GUI_INVALIDATE, surface->window_id, x, y, w, h);
    }
    return APAL_OK;
}

apal_status_t apal_surface_destroy(apal_surface_t *surface) {
    if (!surface) return APAL_ERR_INVALID_PARAM;
    if (surface->window_id > 0) {
        __syscall1(SYS_GUI_DESTROY_WINDOW, surface->window_id);
    } else if (surface->pixel_buffer) {
        atoms_sys_munmap(surface->pixel_buffer, (size_t)surface->width * surface->height * 4);
    }
    memset(surface, 0, sizeof(apal_surface_t));
    return APAL_OK;
}
