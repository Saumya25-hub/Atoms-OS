/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Graphics Surface Adapter (Chromium Viz / Skia / Ozone)
 */

#ifndef ATOMS_APAL_SURFACE_H
#define ATOMS_APAL_SURFACE_H

#include "../include/apal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef apal_handle_t apal_surface_handle_t;

typedef struct {
    apal_surface_handle_t handle;
    uint32_t window_id;
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    uint32_t *pixel_buffer;
} apal_surface_t;

apal_status_t apal_surface_create(uint32_t width, uint32_t height, const char *title, apal_surface_t *out_surface);
apal_status_t apal_surface_present(apal_surface_t *surface, int32_t x, int32_t y, int32_t w, int32_t h);
apal_status_t apal_surface_destroy(apal_surface_t *surface);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_SURFACE_H */
