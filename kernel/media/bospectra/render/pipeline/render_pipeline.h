#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H

#include "../include/bospectra_render.h"
#include "../backends/render_backend.h"

bospectra_error_t bospectra_render_pipeline_process(const BOSPECTRA_RenderBackend* backend, void* surface_ctx, const BOSFrame* frame, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags);

#endif // RENDER_PIPELINE_H
