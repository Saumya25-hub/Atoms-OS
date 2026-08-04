#include "software_backend.h"
#include "../../texture_pool/texture_pool.h"
#include "../../../memory/bospectra_memory.h"
#include "../../../debug/bospectra_debug.h"
#include "kernel/ui/boimage/boimage.h"
#include "kernel/core/lib/include/string.h"

typedef struct {
    uint32_t   window_id;
    uint32_t   width;
    uint32_t   height;
    BOTexture* current_texture;
    BOImage    surface_image;
} SoftwareSurfaceContext;

static bospectra_error_t sw_init(void) {
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t sw_create_surface(void** surface_ctx, uint32_t window_id, uint32_t width, uint32_t height) {
    if (!surface_ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    SoftwareSurfaceContext* ctx = (SoftwareSurfaceContext*)bospectra_mem_alloc(sizeof(SoftwareSurfaceContext), "SWRenderSurface");
    if (!ctx) return BOSPECTRA_ERR_OUT_OF_MEMORY;

    memset(ctx, 0, sizeof(SoftwareSurfaceContext));
    ctx->window_id = window_id;
    ctx->width = (width > 0) ? width : 1920;
    ctx->height = (height > 0) ? height : 1080;

    bospectra_error_t err = bospectra_texture_acquire(ctx->width, ctx->height, &ctx->current_texture);
    if (err != BOSPECTRA_SUCCESS || !ctx->current_texture) {
        bospectra_mem_free(ctx);
        return BOSPECTRA_ERR_OUT_OF_MEMORY;
    }

    memset(ctx->current_texture->data, 0, ctx->width * ctx->height * 4);

    ctx->surface_image.width = ctx->width;
    ctx->surface_image.height = ctx->height;
    ctx->surface_image.format = 0; // BMP/ARGB
    ctx->surface_image.pixels = ctx->current_texture->data;

    *surface_ctx = ctx;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t sw_upload_frame(void* surface_ctx, const BOSFrame* frame) {
    SoftwareSurfaceContext* ctx = (SoftwareSurfaceContext*)surface_ctx;
    if (!ctx || !frame || !frame->data[0]) {
        bospectra_trace_str("TRACE 12 — Software Backend", "Upload Frame FAILED (Invalid Context or Frame)");
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    uint32_t f_w = (frame->width  > 0) ? frame->width  : 640;
    uint32_t f_h = (frame->height > 0) ? frame->height : 360;

    /* Re-acquire BOTexture if dimensions changed */
    if (ctx->width != f_w || ctx->height != f_h || !ctx->current_texture) {
        if (ctx->current_texture) {
            bospectra_texture_release(ctx->current_texture);
            ctx->current_texture = NULL;
        }
        ctx->width  = f_w;
        ctx->height = f_h;
        bospectra_texture_acquire(ctx->width, ctx->height, &ctx->current_texture);
        if (!ctx->current_texture) return BOSPECTRA_ERR_OUT_OF_MEMORY;

        memset(ctx->current_texture->data, 0, ctx->width * ctx->height * 4);
        ctx->surface_image.width  = ctx->width;
        ctx->surface_image.height = ctx->height;
        ctx->surface_image.pixels = ctx->current_texture->data;
    }

    uint32_t* dst = (uint32_t*)ctx->current_texture->data;

    /* ── YUV420P: inline BT.601 → ARGB32 conversion ─────────────────── */
    if (frame->format == BOSPECTRA_PIXEL_FORMAT_YUV420P &&
        frame->data[1] != NULL && frame->data[2] != NULL) {

        /* Strictly use frame->linesize[] strides per row, never assume raw width (Step 2 Bug 2 Fix) */
        uint32_t y_stride = (frame->linesize[0] > 0) ? (uint32_t)frame->linesize[0] : f_w;
        uint32_t u_stride = (frame->linesize[1] > 0) ? (uint32_t)frame->linesize[1] : (f_w / 2);
        uint32_t v_stride = (frame->linesize[2] > 0) ? (uint32_t)frame->linesize[2] : (f_w / 2);
        uint32_t dst_pitch_pixels = (ctx->current_texture->width > 0) ? ctx->current_texture->width : f_w;

        const uint8_t* y_plane = frame->data[0];
        const uint8_t* u_plane = frame->data[1];
        const uint8_t* v_plane = frame->data[2];

        for (uint32_t row = 0; row < f_h; row++) {
            const uint8_t* y_row = y_plane + (size_t)row * y_stride;
            const uint8_t* u_row = u_plane + ((size_t)row / 2) * u_stride;
            const uint8_t* v_row = v_plane + ((size_t)row / 2) * v_stride;
            uint32_t* dst_row = dst + (size_t)row * dst_pitch_pixels;

            for (uint32_t col = 0; col < f_w; col++) {
                int32_t Y  = (int32_t)y_row[col];
                int32_t Cb = (int32_t)u_row[col / 2] - 128;
                int32_t Cr = (int32_t)v_row[col / 2] - 128;

                /* JFIF Full-Range BT.601 conversion (fixed-point x1024) */
                int32_t r = Y + ((1402 * Cr) >> 10);
                int32_t g = Y - ((344 * Cb + 714 * Cr) >> 10);
                int32_t b = Y + ((1772 * Cb) >> 10);

                /* Clamp to [0, 255] */
                r = r < 0 ? 0 : (r > 255 ? 255 : r);
                g = g < 0 ? 0 : (g > 255 ? 255 : g);
                b = b < 0 ? 0 : (b > 255 ? 255 : b);

                /* Pack as 0xFFRRGGBB (ARGB32) */
                dst_row[col] = 0xFF000000U | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
            }
        }
        bospectra_trace_str("Upload Result", "SUCCESS (Inline YUV420P→ARGB32 BT.601)");
        return BOSPECTRA_SUCCESS;
    }

    /* ── ARGB32 / RGB: pitch-aligned row copy ────────────────────────── */
    uint32_t src_pitch   = (frame->linesize[0] > 0) ? frame->linesize[0] : (f_w * 4);
    uint32_t dst_pitch   = f_w * 4;
    uint32_t copy_bytes  = (src_pitch < dst_pitch) ? src_pitch : dst_pitch;

    const uint8_t* src_ptr = frame->data[0];
    uint8_t*       dst_ptr = (uint8_t*)ctx->current_texture->data;

    for (uint32_t y = 0; y < f_h; y++) {
        memcpy(dst_ptr + y * dst_pitch, src_ptr + y * src_pitch, copy_bytes);
    }

    bospectra_trace_str("Upload Result", "SUCCESS (ARGB32 Row-Copy)");
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t sw_present(void* surface_ctx, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags) {
    (void)flags;
    SoftwareSurfaceContext* ctx = (SoftwareSurfaceContext*)surface_ctx;
    if (!ctx || !ctx->current_texture) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    bospectra_trace_str("TRACE 12 — Software Backend", "Present Called");

    // Use BOImage_DrawEx to blit texture to BWE Compositor surface
    int32_t draw_w = (w > 0) ? w : (int32_t)ctx->width;
    int32_t draw_h = (h > 0) ? h : (int32_t)ctx->height;

    BOImage_DrawEx(&ctx->surface_image, x, y, draw_w, draw_h, BO_FILTER_BILINEAR);
    bospectra_trace_str("Present Success", "TRUE");

    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t sw_destroy_surface(void* surface_ctx) {
    SoftwareSurfaceContext* ctx = (SoftwareSurfaceContext*)surface_ctx;
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (ctx->current_texture) {
        bospectra_texture_release(ctx->current_texture);
        ctx->current_texture = NULL;
    }

    bospectra_mem_free(ctx);
    return BOSPECTRA_SUCCESS;
}

static void sw_shutdown(void) {
}

bospectra_error_t bospectra_software_backend_get_pixels(void* surface_ctx, uint32_t** out_pixels, uint32_t* out_w, uint32_t* out_h) {
    SoftwareSurfaceContext* ctx = (SoftwareSurfaceContext*)surface_ctx;
    if (!ctx || !ctx->current_texture || !out_pixels) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    *out_pixels = (uint32_t*)ctx->current_texture->data;
    if (out_w) *out_w = ctx->width;
    if (out_h) *out_h = ctx->height;
    return BOSPECTRA_SUCCESS;
}

// Software Render Backend Driver Vtable Definition
const BOSPECTRA_RenderBackend g_software_render_backend = {
    .backend_name    = "Software",
    .backend_type    = BOSPECTRA_RENDER_BACKEND_SOFTWARE,
    .init            = sw_init,
    .create_surface  = sw_create_surface,
    .upload_frame    = sw_upload_frame,
    .present         = sw_present,
    .destroy_surface = sw_destroy_surface,
    .shutdown        = sw_shutdown
};
