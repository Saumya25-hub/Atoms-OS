#include "frame_pool.h"
#include "../../memory/bospectra_memory.h"
#include "../../include/bospectra_errors.h"
#include "../../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static BOSFrame g_frame_pool[BOSPECTRA_FRAME_POOL_SIZE];
static uint32_t g_frame_pool_active = 0;
static uint32_t g_frame_pool_peak = 0;
static bool     g_frame_pool_initialized = false;

void bospectra_frame_ref(BOSFrame* frame) {
    if (frame && frame->is_allocated && frame->ref_count > 0) {
        frame->ref_count++;
    }
}

bospectra_error_t bospectra_frame_unref(BOSFrame* frame) {
    if (!frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (!frame->is_allocated || frame->ref_count == 0) {
        return BOSPECTRA_ERR_HANDLE_INVALID;
    }

    frame->ref_count--;
    if (frame->ref_count == 0) {
        return bospectra_frame_release(frame);
    }
    return BOSPECTRA_SUCCESS;
}

bool bospectra_frame_is_valid(const BOSFrame* frame) {
    return (frame != NULL && frame->is_allocated && frame->ref_count > 0);
}

void bospectra_frame_pool_init(void) {
    memset(g_frame_pool, 0, sizeof(g_frame_pool));
    
    // Initialize pool slots; frame buffers are allocated on-demand to prevent heap exhaustion
    for (uint32_t i = 0; i < BOSPECTRA_FRAME_POOL_SIZE; i++) {
        g_frame_pool[i].frame_id = i + 1;
        g_frame_pool[i].pool_index = i;
        g_frame_pool[i].buffer_size = 0;
        g_frame_pool[i].data[0] = NULL;
        g_frame_pool[i].is_allocated = false;
        g_frame_pool[i].ref_count = 0;
    }

    g_frame_pool_active = 0;
    g_frame_pool_peak = 0;
    g_frame_pool_initialized = true;
}

void bospectra_frame_pool_shutdown(void) {
    for (uint32_t i = 0; i < BOSPECTRA_FRAME_POOL_SIZE; i++) {
        if (g_frame_pool[i].data[0]) {
            bospectra_mem_free(g_frame_pool[i].data[0]);
            g_frame_pool[i].data[0] = NULL;
        }
    }
    memset(g_frame_pool, 0, sizeof(g_frame_pool));
    g_frame_pool_active = 0;
    g_frame_pool_peak = 0;
    g_frame_pool_initialized = false;
}

bospectra_error_t bospectra_frame_acquire(uint32_t width, uint32_t height, bospectra_pixel_format_t format, BOSFrame** out_frame) {
    if (!g_frame_pool_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!out_frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    uint32_t w = (width > 0) ? width : BOSPECTRA_DEFAULT_FRAME_WIDTH;
    uint32_t h = (height > 0) ? height : BOSPECTRA_DEFAULT_FRAME_HEIGHT;
    bospectra_pixel_format_t fmt = (format != BOSPECTRA_PIXEL_FORMAT_UNKNOWN) ? format : BOSPECTRA_PIXEL_FORMAT_ARGB32;

    uint32_t needed_size = 0;
    uint32_t padded_w = (w + 15U) & ~15U;
    uint32_t padded_h = (h + 15U) & ~15U;
    if (fmt == BOSPECTRA_PIXEL_FORMAT_YUV420P) {
        needed_size = (padded_w * padded_h) + ((padded_w / 2) * (padded_h / 2) * 2);
    } else {
        needed_size = w * h * 4;
    }

    for (uint32_t i = 0; i < BOSPECTRA_FRAME_POOL_SIZE; i++) {
        if (!g_frame_pool[i].is_allocated) {
            BOSFrame* f = &g_frame_pool[i];

            if (!f->data[0] || f->buffer_size < needed_size) {
                if (f->data[0]) {
                    bospectra_mem_free(f->data[0]);
                    f->data[0] = NULL;
                }
                f->data[0] = (uint8_t*)bospectra_mem_alloc_aligned(needed_size, BOSPECTRA_DEFAULT_ALIGNMENT, "FrameBuf");
                if (!f->data[0]) {
                    return BOSPECTRA_ERR_OUT_OF_MEMORY;
                }
                f->buffer_size = needed_size;
            }

            uint8_t* base_buf = f->data[0];
            f->width = w;
            f->height = h;
            f->format = fmt;

            if (f->format == BOSPECTRA_PIXEL_FORMAT_YUV420P) {
                f->linesize[0] = padded_w;
                f->linesize[1] = padded_w / 2;
                f->linesize[2] = padded_w / 2;
                f->linesize[3] = 0;
                f->data[0] = base_buf;
                f->data[1] = base_buf + (padded_w * padded_h);
                f->data[2] = base_buf + (padded_w * padded_h) + ((padded_w / 2) * (padded_h / 2));
                f->data[3] = NULL;
            } else {
                f->linesize[0] = w * 4;
                f->linesize[1] = 0;
                f->linesize[2] = 0;
                f->linesize[3] = 0;
                f->data[0] = base_buf;
                f->data[1] = NULL;
                f->data[2] = NULL;
                f->data[3] = NULL;
            }

            f->pts = 0;
            f->dts = 0;
            f->duration_us = 0;
            f->flags = 0;
            f->ref_count = 1;
            f->is_allocated = true;

            g_frame_pool_active++;
            if (g_frame_pool_active > g_frame_pool_peak) {
                g_frame_pool_peak = g_frame_pool_active;
            }

            bospectra_trace_str("TRACE 9 — Frame Pool", "Acquire");
            bospectra_trace_u32("Pool Index", i);
            bospectra_trace_hex("Address", (uint64_t)(uintptr_t)f->data[0]);
            bospectra_trace_u32("Width", f->width);
            bospectra_trace_u32("Height", f->height);
            bospectra_trace_str("Format", (f->format == BOSPECTRA_PIXEL_FORMAT_YUV420P) ? "YUV420P" : "ARGB32");
            bospectra_trace_u32("Ref Count", f->ref_count);

            *out_frame = f;
            return BOSPECTRA_SUCCESS;
        }
    }

    *out_frame = NULL;
    return BOSPECTRA_ERR_OUT_OF_MEMORY; // Frame pool exhausted
}

bospectra_error_t bospectra_frame_release(BOSFrame* frame) {
    if (!g_frame_pool_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!frame || !frame->is_allocated) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    bospectra_trace_str("TRACE 9 — Frame Pool", "Release");
    bospectra_trace_u32("Pool Index", frame->pool_index);
    bospectra_trace_hex("Address", (uint64_t)(uintptr_t)frame->data[0]);
    bospectra_trace_u32("Ref Count", 0);

    uint8_t* base_buf = frame->data[0];
    uint32_t idx = frame->pool_index;
    uint32_t fid = frame->frame_id;
    uint32_t bsz = frame->buffer_size;

    memset(frame, 0, sizeof(BOSFrame));
    frame->frame_id = fid;
    frame->pool_index = idx;
    frame->buffer_size = bsz;
    frame->data[0] = base_buf;
    frame->is_allocated = false;
    frame->ref_count = 0;

    if (g_frame_pool_active > 0) {
        g_frame_pool_active--;
    }

    return BOSPECTRA_SUCCESS;
}

void bospectra_frame_pool_get_counts(uint32_t* active, uint32_t* peak, uint32_t* capacity) {
    if (active) *active = g_frame_pool_active;
    if (peak) *peak = g_frame_pool_peak;
    if (capacity) *capacity = BOSPECTRA_FRAME_POOL_SIZE;
}
