/*
 * BOSPECTRA V3 — Render Manager Implementation
 * kernel/media/bospectra/manager/render_manager.c
 */

#include "render_manager.h"
#include "../registry/renderer_registry.h"
#include "../render/backends/software/software_backend.h"
#include "../render/backends/opengl/opengl_backend.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

extern const BOSPECTRA_RenderBackend g_software_render_backend;
extern const BOSPECTRA_RenderBackend g_opengl_render_backend;

static bool g_render_manager_initialized = false;

void bospectra_render_manager_init(void) {
    BOSPECTRA_Render_Init();
    bospectra_v3_renderer_registry_init();

    BOSPECTRA_RendererRecord rec_sw = {
        .backend_name = "Software", .backend_type = BOSPECTRA_RENDER_BACKEND_SOFTWARE,
        .priority = 100, .backend_vtable = &g_software_render_backend
    };
    BOSPECTRA_RendererRecord rec_gl = {
        .backend_name = "OpenGL", .backend_type = BOSPECTRA_RENDER_BACKEND_OPENGL,
        .priority = 90, .backend_vtable = &g_opengl_render_backend
    };

    bospectra_v3_renderer_register(&rec_sw);
    bospectra_v3_renderer_register(&rec_gl);

    g_render_manager_initialized = true;
    bospectra_log("RENDER_MANAGER", "BOSPECTRA V3 Render Manager initialized (Software & OpenGL Renderers Registered).");
}

void bospectra_render_manager_shutdown(void) {
    bospectra_v3_renderer_registry_shutdown();
    BOSPECTRA_Render_Shutdown();
    g_render_manager_initialized = false;
}

bospectra_error_t bospectra_render_create_target(uint32_t window_id, uint32_t width, uint32_t height, bospectra_render_session_id_t* out_render_id) {
    if (!g_render_manager_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!out_render_id) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    const BOSPECTRA_RendererRecord* rec = bospectra_v3_renderer_resolve_best(BOSPECTRA_RENDER_BACKEND_SOFTWARE);

    BOSPECTRA_RenderSpec spec;
    memset(&spec, 0, sizeof(spec));
    spec.backend = rec ? rec->backend_type : BOSPECTRA_RENDER_BACKEND_SOFTWARE;
    spec.filter = BOSPECTRA_SCALING_FILTER_BILINEAR;
    spec.rotation = BOSPECTRA_ROTATION_0;
    spec.target_window_id = window_id;
    spec.flags = BOSPECTRA_RENDER_FLAG_VSYNC;

    (void)width;
    (void)height;

    return BOSPECTRA_Render_OpenSession(&spec, out_render_id);
}

bospectra_error_t bospectra_render_present(bospectra_render_session_id_t render_id, const BOSFrame* frame, int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!g_render_manager_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return BOSPECTRA_Render_PresentFrame(render_id, frame, x, y, w, h);
}

bospectra_error_t bospectra_render_destroy_target(bospectra_render_session_id_t render_id) {
    if (!g_render_manager_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return BOSPECTRA_Render_CloseSession(render_id);
}
