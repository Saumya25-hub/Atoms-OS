#include "render_pipeline.h"
#include "../../include/bospectra_errors.h"

bospectra_error_t bospectra_render_pipeline_process(const BOSPECTRA_RenderBackend* backend, void* surface_ctx, const BOSFrame* frame, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags) {
    if (!backend || !surface_ctx || !frame || !frame->data[0]) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    // Zero-width & zero-height boundary protection
    if (frame->width == 0 || frame->height == 0) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    // Frame validity guard
    if (!bospectra_frame_is_valid(frame)) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    // Upload converted BOSFrame -> BOTexture surface
    bospectra_error_t err = backend->upload_frame(surface_ctx, frame);
    if (err != BOSPECTRA_SUCCESS) {
        return err;
    }

    // Present surface via backend blitter
    return backend->present(surface_ctx, x, y, w, h, flags);
}
