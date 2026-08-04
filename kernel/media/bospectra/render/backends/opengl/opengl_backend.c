#include "opengl_backend.h"
#include "../../texture_pool/texture_pool.h"
#include "../../../memory/bospectra_memory.h"
#include "kernel/ui/boimage/boimage.h"
#include "kernel/core/lib/include/string.h"

typedef struct {
    uint32_t   window_id;
    uint32_t   width;
    uint32_t   height;
    BOTexture* current_texture;
    BOImage    surface_image;
} GLSurfaceContext;

static bospectra_error_t gl_init(void) {
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t gl_create_surface(void** surface_ctx, uint32_t window_id, uint32_t width, uint32_t height) {
    if (!surface_ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    GLSurfaceContext* ctx = (GLSurfaceContext*)bospectra_mem_alloc(sizeof(GLSurfaceContext), "GLRenderSurface");
    if (!ctx) return BOSPECTRA_ERR_OUT_OF_MEMORY;

    memset(ctx, 0, sizeof(GLSurfaceContext));
    ctx->window_id = window_id;
    ctx->width = (width > 0) ? width : 1920;
    ctx->height = (height > 0) ? height : 1080;

    bospectra_error_t err = bospectra_texture_acquire(ctx->width, ctx->height, &ctx->current_texture);
    if (err != BOSPECTRA_SUCCESS || !ctx->current_texture) {
        bospectra_mem_free(ctx);
        return BOSPECTRA_ERR_OUT_OF_MEMORY;
    }

    ctx->surface_image.width = ctx->width;
    ctx->surface_image.height = ctx->height;
    ctx->surface_image.format = 0;
    ctx->surface_image.pixels = ctx->current_texture->data;

    *surface_ctx = ctx;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t gl_upload_frame(void* surface_ctx, const BOSFrame* frame) {
    GLSurfaceContext* ctx = (GLSurfaceContext*)surface_ctx;
    if (!ctx || !frame || !frame->data[0] || !ctx->current_texture) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    // OpenGL Staging Buffer Upload
    size_t bytes_to_copy = frame->width * frame->height * 4;
    if (bytes_to_copy > (1920 * 1080 * 4)) {
        bytes_to_copy = 1920 * 1080 * 4;
    }

    memcpy(ctx->current_texture->data, frame->data[0], bytes_to_copy);
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t gl_present(void* surface_ctx, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags) {
    (void)flags;
    GLSurfaceContext* ctx = (GLSurfaceContext*)surface_ctx;
    if (!ctx || !ctx->current_texture) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    int32_t draw_w = (w > 0) ? w : (int32_t)ctx->width;
    int32_t draw_h = (h > 0) ? h : (int32_t)ctx->height;

    BOImage_DrawEx(&ctx->surface_image, x, y, draw_w, draw_h, BO_FILTER_BILINEAR);

    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t gl_destroy_surface(void* surface_ctx) {
    GLSurfaceContext* ctx = (GLSurfaceContext*)surface_ctx;
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (ctx->current_texture) {
        bospectra_texture_release(ctx->current_texture);
        ctx->current_texture = NULL;
    }

    bospectra_mem_free(ctx);
    return BOSPECTRA_SUCCESS;
}

static void gl_shutdown(void) {
}

// OpenGL Render Backend Driver Vtable Definition
const BOSPECTRA_RenderBackend g_opengl_render_backend = {
    .backend_name    = "OpenGL",
    .backend_type    = BOSPECTRA_RENDER_BACKEND_OPENGL,
    .init            = gl_init,
    .create_surface  = gl_create_surface,
    .upload_frame    = gl_upload_frame,
    .present         = gl_present,
    .destroy_surface = gl_destroy_surface,
    .shutdown        = gl_shutdown
};
