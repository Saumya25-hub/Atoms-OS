/**
 * @file display_capabilities.c
 * @brief ATOMS OS Display Intelligence Engine - Capability Analysis Implementation
 */

#include "display_capabilities.h"
#include "display_detection.h"

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

static void add_mode(DIE_CapabilityList* list, uint32_t id, uint32_t w, uint32_t h, uint32_t bpp, uint32_t hz) {
    if (list->mode_count >= DIE_MAX_MODES) return;
    uint32_t idx = list->mode_count++;
    list->modes[idx].mode_id = id;
    list->modes[idx].width = w;
    list->modes[idx].height = h;
    list->modes[idx].bpp = bpp;
    list->modes[idx].pitch_bytes = w * (bpp / 8);
    list->modes[idx].refresh_rate_hz = hz;
    list->modes[idx].is_supported = true;
    list->modes[idx].is_preferred = false;
    list->modes[idx].policy_score = 0;
}

const char* DIE_Capabilities_GetAspectRatioString(uint32_t width, uint32_t height) {
    if (height == 0) return "Unknown";
    uint32_t ratio_x100 = (width * 100) / height;
    if (ratio_x100 >= 170 && ratio_x100 <= 180) return "16:9 (Widescreen)";
    if (ratio_x100 >= 155 && ratio_x100 <= 165) return "16:10 (Widescreen)";
    if (ratio_x100 >= 130 && ratio_x100 <= 136) return "4:3 (Standard)";
    return "Custom Aspect";
}

void DIE_Capabilities_Collect(DIE_DisplayInfo* info) {
    if (!info) return;

    info->capabilities.mode_count = 0;
    info->capabilities.preferred_index = 0;

    /* Populate standard hardware VBE / Framebuffer modes */
    add_mode(&info->capabilities, 1, 1920, 1080, 32, 60);
    add_mode(&info->capabilities, 2, 1600, 1200, 32, 60);
    add_mode(&info->capabilities, 3, 1600,  900, 32, 60);
    add_mode(&info->capabilities, 4, 1440,  900, 32, 60);
    add_mode(&info->capabilities, 5, 1366,  768, 32, 60);
    add_mode(&info->capabilities, 6, 1280,  720, 32, 60);
    add_mode(&info->capabilities, 7, 1024,  768, 32, 60);

    /* Ensure live active mode is recorded */
    uint32_t live_w = (g_kernel_screen_width > 0) ? g_kernel_screen_width : 1280;
    uint32_t live_h = (g_kernel_screen_height > 0) ? g_kernel_screen_height : 720;
    
    bool found = false;
    for (uint32_t i = 0; i < info->capabilities.mode_count; i++) {
        if (info->capabilities.modes[i].width == live_w && info->capabilities.modes[i].height == live_h) {
            found = true;
            break;
        }
    }
    if (!found) {
        add_mode(&info->capabilities, 0, live_w, live_h, 32, 60);
    }

    info->vram_size_bytes = DIE_Detection_GetVRAMSize();
}
