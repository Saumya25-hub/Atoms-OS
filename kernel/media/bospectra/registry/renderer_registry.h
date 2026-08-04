/*
 * BOSPECTRA V3 — Dynamic Renderer Registry
 * kernel/media/bospectra/registry/renderer_registry.h
 *
 * Single source of truth for Software, OpenGL, Vulkan, and HW Video renderer backend registration and capability resolution.
 */

#ifndef BOSPECTRA_V3_RENDERER_REGISTRY_H
#define BOSPECTRA_V3_RENDERER_REGISTRY_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include "../render/include/bospectra_render_types.h"
#include "../render/backends/render_backend.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BOSPECTRA_MAX_RENDERER_DRIVERS 8U

typedef struct {
    const char*                backend_name;
    bospectra_render_backend_t backend_type;
    uint32_t                   priority;
    const BOSPECTRA_RenderBackend* backend_vtable;
} BOSPECTRA_RendererRecord;

void bospectra_v3_renderer_registry_init(void);
void bospectra_v3_renderer_registry_shutdown(void);

bospectra_error_t bospectra_v3_renderer_register(const BOSPECTRA_RendererRecord* record);
const BOSPECTRA_RendererRecord* bospectra_v3_renderer_resolve_best(bospectra_render_backend_t preferred_type);
uint32_t bospectra_v3_renderer_get_registered(const BOSPECTRA_RendererRecord** out_records, uint32_t max_count);

#endif /* BOSPECTRA_V3_RENDERER_REGISTRY_H */
