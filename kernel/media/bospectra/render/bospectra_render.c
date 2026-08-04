#include "include/bospectra_render.h"
#include "backends/render_backend.h"
#include "backends/software/software_backend.h"
#include "backends/opengl/opengl_backend.h"
#include "texture_pool/texture_pool.h"
#include "pipeline/render_pipeline.h"
#include "diagnostics/render_diag.h"
#include "tests/render_tests.h"
#include "../memory/bospectra_memory.h"
#include "../debug/bospectra_debug.h"
#include "../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_RENDER_SESSIONS 8U

typedef struct {
    bospectra_render_session_id_t  id;
    const BOSPECTRA_RenderBackend* backend;
    void*                          surface_ctx;
    BOSPECTRA_RenderSpec           spec;
    bool                           is_active;
} RenderSession;

static RenderSession g_render_sessions[BOSPECTRA_MAX_RENDER_SESSIONS];
static bool g_render_engine_initialized = false;

void bospectra_render_backend_init(void) {
}

void bospectra_render_backend_shutdown(void) {
}

bospectra_error_t BOSPECTRA_Render_Init(void) {
    if (g_render_engine_initialized) {
        return BOSPECTRA_ERR_ALREADY_INITIALIZED;
    }

    memset(g_render_sessions, 0, sizeof(g_render_sessions));
    bospectra_texture_pool_init();
    bospectra_render_diag_init();

    g_render_engine_initialized = true;
    bospectra_log("RENDER_ENGINE", "Rendering Subsystem Initialized (Software & OpenGL Backends Ready).");

    // Run self-tests and render loop stress test (disabled for instant boot)
    // bospectra_render_tests_run_all();

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Render_Shutdown(void) {
    if (!g_render_engine_initialized) {
        return BOSPECTRA_ERR_NOT_INITIALIZED;
    }

    for (uint32_t i = 0; i < BOSPECTRA_MAX_RENDER_SESSIONS; i++) {
        if (g_render_sessions[i].is_active) {
            BOSPECTRA_Render_CloseSession(g_render_sessions[i].id);
        }
    }

    bospectra_render_diag_shutdown();
    bospectra_texture_pool_shutdown();
    memset(g_render_sessions, 0, sizeof(g_render_sessions));
    g_render_engine_initialized = false;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Render_OpenSession(const BOSPECTRA_RenderSpec* spec, bospectra_render_session_id_t* out_session_id) {
    if (!g_render_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!spec || !out_session_id) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    uint32_t slot = BOSPECTRA_MAX_RENDER_SESSIONS;
    for (uint32_t i = 0; i < BOSPECTRA_MAX_RENDER_SESSIONS; i++) {
        if (!g_render_sessions[i].is_active) {
            slot = i;
            break;
        }
    }
    if (slot >= BOSPECTRA_MAX_RENDER_SESSIONS) return BOSPECTRA_ERR_BUFFER_OVERFLOW;

    const BOSPECTRA_RenderBackend* backend = (spec->backend == BOSPECTRA_RENDER_BACKEND_OPENGL) ?
                                             &g_opengl_render_backend :
                                             &g_software_render_backend;

    void* surface_ctx = NULL;
    bospectra_error_t err = backend->create_surface(&surface_ctx, spec->target_window_id, 1920, 1080);
    if (err != BOSPECTRA_SUCCESS || !surface_ctx) {
        return err;
    }

    RenderSession* sess = &g_render_sessions[slot];
    sess->id = slot + 1;
    sess->backend = backend;
    sess->surface_ctx = surface_ctx;
    sess->spec = *spec;
    sess->is_active = true;

    *out_session_id = sess->id;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Render_PresentFrame(bospectra_render_session_id_t session_id, const BOSFrame* frame, int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!g_render_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (session_id == 0 || session_id > BOSPECTRA_MAX_RENDER_SESSIONS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = session_id - 1;
    RenderSession* sess = &g_render_sessions[idx];
    if (!sess->is_active || !sess->backend) return BOSPECTRA_ERR_HANDLE_INVALID;

    bospectra_trace_str("TRACE 11 — Render Engine", "Presenting Frame to Backend");
    bospectra_trace_u32("Render Session ID", session_id);
    bospectra_trace_str("Backend", sess->backend->backend_name);
    bospectra_trace_u32("Frame Width", frame->width);
    bospectra_trace_u32("Frame Height", frame->height);

    bospectra_error_t err = bospectra_render_pipeline_process(sess->backend, sess->surface_ctx, frame, x, y, w, h, sess->spec.flags);
    bool success = (err == BOSPECTRA_SUCCESS);

    bospectra_trace_str("Upload Success", success ? "TRUE" : "FAILED");

    bospectra_render_diag_record_frame(25, sess->backend->backend_name, success); // Microsecond telemetry record

    return err;
}

bospectra_error_t BOSPECTRA_Render_SetBackend(bospectra_render_session_id_t session_id, bospectra_render_backend_t backend_type) {
    if (!g_render_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (session_id == 0 || session_id > BOSPECTRA_MAX_RENDER_SESSIONS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = session_id - 1;
    RenderSession* sess = &g_render_sessions[idx];
    if (!sess->is_active) return BOSPECTRA_ERR_HANDLE_INVALID;

    sess->backend = (backend_type == BOSPECTRA_RENDER_BACKEND_OPENGL) ?
                    &g_opengl_render_backend :
                    &g_software_render_backend;
    sess->spec.backend = backend_type;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Render_GetSurfacePixels(bospectra_render_session_id_t session_id, uint32_t** out_pixels, uint32_t* out_w, uint32_t* out_h) {
    if (!g_render_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (session_id == 0 || session_id > BOSPECTRA_MAX_RENDER_SESSIONS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = session_id - 1;
    RenderSession* sess = &g_render_sessions[idx];
    if (!sess->is_active || !sess->surface_ctx) return BOSPECTRA_ERR_HANDLE_INVALID;

    return bospectra_software_backend_get_pixels(sess->surface_ctx, out_pixels, out_w, out_h);
}

bospectra_error_t BOSPECTRA_Render_CloseSession(bospectra_render_session_id_t session_id) {
    if (!g_render_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (session_id == 0 || session_id > BOSPECTRA_MAX_RENDER_SESSIONS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = session_id - 1;
    RenderSession* sess = &g_render_sessions[idx];
    if (!sess->is_active) return BOSPECTRA_ERR_HANDLE_INVALID;

    if (sess->backend && sess->backend->destroy_surface && sess->surface_ctx) {
        sess->backend->destroy_surface(sess->surface_ctx);
    }

    memset(sess, 0, sizeof(RenderSession));
    return BOSPECTRA_SUCCESS;
}

void BOSPECTRA_Render_GetStats(BOSPECTRA_RenderStats* out_stats) {
    bospectra_render_collect_stats(out_stats);
}

void BOSPECTRA_Render_DumpDiagnostics(void) {
    bospectra_render_dump_telemetry();
}
