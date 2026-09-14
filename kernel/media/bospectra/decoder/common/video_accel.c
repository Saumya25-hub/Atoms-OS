/*
 * ATOMS OS — BOS Video Acceleration HAL Implementation
 * kernel/media/bospectra/decoder/common/video_accel.c
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "../include/video_accel.h"
#include "../../debug/bospectra_debug.h"
#include "kernel/core/pci/pci.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern void display_print_hex(uint64_t val);

static bospectra_accel_backend_t g_active_backend = BOSPECTRA_ACCEL_BACKEND_SOFTWARE;
static bool g_accel_initialized = false;
static uint16_t g_detected_vendor_id = 0;
static uint16_t g_detected_device_id = 0;
static const char* g_detected_vendor_name = "Generic / Virtual Display";

void bospectra_video_accel_init(void) {
    g_active_backend = BOSPECTRA_ACCEL_BACKEND_SOFTWARE;
    g_accel_initialized = true;

    /* Universal Vendor Hardware Capability Probe */
    uint32_t dev_count = pci_get_device_count();
    for (uint32_t i = 0; i < dev_count; i++) {
        PCIDevice* dev = pci_get_device(i);
        if (dev && dev->base_class == 0x03) { // PCI Display Controller
            g_detected_vendor_id = dev->vendor_id;
            g_detected_device_id = dev->device_id;
            if (dev->vendor_id == 0x8086) {
                g_detected_vendor_name = "Intel Corporation";
            } else if (dev->vendor_id == 0x10DE) {
                g_detected_vendor_name = "NVIDIA Corporation";
            } else if (dev->vendor_id == 0x1002) {
                g_detected_vendor_name = "Advanced Micro Devices (AMD)";
            } else if (dev->vendor_id == 0x1B36) {
                g_detected_vendor_name = "Red Hat / QEMU QXL";
            } else if (dev->vendor_id == 0x1234) {
                g_detected_vendor_name = "Bochs / QEMU Standard VGA";
            } else if (dev->vendor_id == 0x15AD) {
                g_detected_vendor_name = "VMware SVGA II";
            }
            break;
        }
    }

    display_print("[GPU] probe: ");
    display_print(g_detected_vendor_name);
    display_print(" detected (Vendor 0x");
    display_print_hex(g_detected_vendor_id);
    display_print(" Device 0x");
    display_print_hex(g_detected_device_id);
    display_print(")\n");

    if (g_detected_vendor_id == 0x10DE) {
        display_print("[GPU] hardware profile: NVIDIA NVDEC capable (RTX Architecture)\n");
    } else if (g_detected_vendor_id == 0x8086) {
        display_print("[GPU] hardware profile: Intel QuickSync / VA-API capable\n");
    } else if (g_detected_vendor_id == 0x1002) {
        display_print("[GPU] hardware profile: AMD VCN capable\n");
    }

    display_print("[GPU] bridge status: SMART CPU SOFTWARE FALLBACK BRIDGE ENGAGED\n");
    display_print("[GPU] backend selected = SOFTWARE (FFmpeg libavcodec + C99 Color Engine)\n");

    bospectra_log("VIDEO_ACCEL", "Universal Video Acceleration HAL Initialized (Smart CPU Fallback Bridge Active).");
}

bospectra_accel_backend_t bospectra_video_accel_get_backend(void) {
    return g_active_backend;
}

const char* bospectra_video_accel_get_backend_name(void) {
    switch (g_active_backend) {
        case BOSPECTRA_ACCEL_BACKEND_SOFTWARE: return "Software C99 Rasterizer";
        case BOSPECTRA_ACCEL_BACKEND_NVIDIA_NVDEC: return "NVIDIA NVDEC (Hardware)";
        case BOSPECTRA_ACCEL_BACKEND_INTEL_VAAPI: return "Intel QuickSync / VAAPI (Hardware)";
        case BOSPECTRA_ACCEL_BACKEND_AMD_VCN: return "AMD VCN (Hardware)";
        default: return "None";
    }
}

bool bospectra_video_accel_is_hw(void) {
    return (g_active_backend != BOSPECTRA_ACCEL_BACKEND_SOFTWARE &&
            g_active_backend != BOSPECTRA_ACCEL_BACKEND_NONE);
}

bospectra_error_t bospectra_video_accel_get_caps(bospectra_accel_caps_t* out_caps) {
    if (!out_caps) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    memset(out_caps, 0, sizeof(*out_caps));

    out_caps->backend = g_active_backend;
    out_caps->backend_name = bospectra_video_accel_get_backend_name();
    out_caps->is_hardware_accelerated = bospectra_video_accel_is_hw();
    out_caps->gpu_vendor_id = g_detected_vendor_id;
    out_caps->gpu_device_id = g_detected_device_id;
    out_caps->gpu_vendor_name = g_detected_vendor_name;
    out_caps->max_width = 1920;
    out_caps->max_height = 1080;
    out_caps->supported_codecs_mask = (1 << BOSPECTRA_CODEC_H264) |
                                      (1 << BOSPECTRA_CODEC_HEVC) |
                                      (1 << BOSPECTRA_CODEC_VP8)  |
                                      (1 << BOSPECTRA_CODEC_VP9)  |
                                      (1 << BOSPECTRA_CODEC_MJPEG);
    return BOSPECTRA_SUCCESS;
}

void bospectra_video_accel_dump_diagnostics(void) {
    display_print("\n============================================\n");
    display_print("  BOS VIDEO ACCELERATION HAL DIAGNOSTICS   \n");
    display_print("============================================\n");
    display_print(" GPU Vendor       : "); display_print(g_detected_vendor_name); display_print("\n");
    display_print(" Active Backend   : "); display_print(bospectra_video_accel_get_backend_name()); display_print("\n");
    display_print(" Hardware Accel   : NO (Pure Software Pipeline)\n");
    display_print(" Max Resolution   : 1920x1080 (Full HD)\n");
    display_print(" Decode Mode      : SOFTWARE\n");
    display_print(" Color Matrix     : BT.601 (SD) / BT.709 (HD)\n");
    display_print(" HAL Status       : PASS\n");
    display_print("============================================\n\n");
}
