/*
 * ATOMS OS — BOS Video Acceleration HAL
 * kernel/media/bospectra/decoder/include/video_accel.h
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef BOSPECTRA_VIDEO_ACCEL_H
#define BOSPECTRA_VIDEO_ACCEL_H

#include <stdint.h>
#include <stdbool.h>
#include "bospectra_codec_types.h"
#include "../../include/bospectra_errors.h"

typedef enum {
    BOSPECTRA_ACCEL_BACKEND_NONE = 0,
    BOSPECTRA_ACCEL_BACKEND_SOFTWARE,
    BOSPECTRA_ACCEL_BACKEND_NVIDIA_NVDEC,
    BOSPECTRA_ACCEL_BACKEND_INTEL_VAAPI,
    BOSPECTRA_ACCEL_BACKEND_AMD_VCN
} bospectra_accel_backend_t;

typedef enum {
    BOSPECTRA_SURFACE_YUV420P = 0,
    BOSPECTRA_SURFACE_NV12,
    BOSPECTRA_SURFACE_P010,
    BOSPECTRA_SURFACE_ARGB32
} bospectra_surface_format_t;

typedef struct {
    bospectra_accel_backend_t backend;
    const char*               backend_name;
    bool                      is_hardware_accelerated;
    uint16_t                  gpu_vendor_id;
    uint16_t                  gpu_device_id;
    const char*               gpu_vendor_name;
    uint32_t                  max_width;
    uint32_t                  max_height;
    uint32_t                  supported_codecs_mask;
} bospectra_accel_caps_t;

void                      bospectra_video_accel_init(void);
bospectra_accel_backend_t bospectra_video_accel_get_backend(void);
const char*               bospectra_video_accel_get_backend_name(void);
bool                      bospectra_video_accel_is_hw(void);
bospectra_error_t         bospectra_video_accel_get_caps(bospectra_accel_caps_t* out_caps);
void                      bospectra_video_accel_dump_diagnostics(void);

#endif /* BOSPECTRA_VIDEO_ACCEL_H */
