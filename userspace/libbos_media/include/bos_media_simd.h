/*
 * ============================================================================
 * ATOMS OS — BOSpectra SIMD Dispatcher & Vector Conversion Engine
 * userspace/libbos_media/include/bos_media_simd.h
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Provides runtime CPUID-detected SIMD acceleration for:
 * - BT.709 YUV420P to ARGB32 conversion with direct non-temporal stores
 * - Scaling coordinate precomputation
 * - Bit-exact scalar reference validation
 * ============================================================================
 */

#ifndef BOS_MEDIA_SIMD_H
#define BOS_MEDIA_SIMD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOS_SIMD_SCALAR = 0,
    BOS_SIMD_SSE2   = 1,
    BOS_SIMD_AVX2   = 2
} BOSMediaSIMDBackend;

typedef struct {
    bool                has_sse2;
    bool                has_avx2;
    BOSMediaSIMDBackend active_backend;
} BOSMediaSIMDCaps;

void                bos_media_simd_init(void);
BOSMediaSIMDBackend bos_media_simd_get_backend(void);
const char*         bos_media_simd_get_backend_name(void);
const BOSMediaSIMDCaps* bos_media_simd_get_caps(void);

/*
 * Converts a planar YUV420P picture directly into target ARGB32 framebuffer.
 * Scales from (src_w x src_h) to (dst_w x dst_h) at destination offset (dst_x, dst_y).
 */
void bos_media_simd_yuv420p_to_argb(
    const uint8_t* y_plane,
    const uint8_t* u_plane,
    const uint8_t* v_plane,
    int src_w,
    int src_h,
    uint32_t* dst_fb,
    int dst_stride_pixels,
    int dst_x,
    int dst_y,
    int dst_w,
    int dst_h
);

/*
 * Reference scalar conversion (guaranteed baseline correctness for testing/fallback)
 */
void bos_media_simd_yuv420p_to_argb_scalar(
    const uint8_t* y_plane,
    const uint8_t* u_plane,
    const uint8_t* v_plane,
    int src_w,
    int src_h,
    uint32_t* dst_fb,
    int dst_stride_pixels,
    int dst_x,
    int dst_y,
    int dst_w,
    int dst_h
);

/*
 * Performance benchmark comparing scalar vs active SIMD backend on dummy frame
 */
void bos_media_simd_benchmark(
    int width,
    int height,
    uint32_t* out_scalar_us,
    uint32_t* out_simd_us
);

#ifdef __cplusplus
}
#endif

#endif /* BOS_MEDIA_SIMD_H */
