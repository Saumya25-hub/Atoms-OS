#ifndef RENDER_BACKEND_H
#define RENDER_BACKEND_H

#include "../include/bospectra_render_types.h"
#include "../../frame_memory/include/bospectra_frame.h"
#include "../../include/bospectra_errors.h"

// Polymorphic Rendering Backend Driver vtable
typedef struct BOSPECTRA_RenderBackend {
    const char*                backend_name;
    bospectra_render_backend_t backend_type;

    // Initialize backend subsystem
    bospectra_error_t (*init)(void);

    // Create rendering surface for target window ID
    bospectra_error_t (*create_surface)(void** surface_ctx, uint32_t window_id, uint32_t width, uint32_t height);

    // Upload converted BOSFrame -> BOTexture surface
    bospectra_error_t (*upload_frame)(void* surface_ctx, const BOSFrame* frame);

    // Scale, rotate, and present surface to BWE compositor
    bospectra_error_t (*present)(void* surface_ctx, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags);

    // Destroy surface & release textures
    bospectra_error_t (*destroy_surface)(void* surface_ctx);

    // Shutdown backend subsystem
    void (*shutdown)(void);
} BOSPECTRA_RenderBackend;

void                           bospectra_render_backend_init(void);
void                           bospectra_render_backend_shutdown(void);
bospectra_error_t              bospectra_render_register_backend(const BOSPECTRA_RenderBackend* backend);
const BOSPECTRA_RenderBackend* bospectra_render_find_backend(bospectra_render_backend_t type);

#endif // RENDER_BACKEND_H
