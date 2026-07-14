/**
 * @file display_layout.c
 * @brief ATOMS OS Display Intelligence Engine - Layout & Subsystem Synchronization Implementation
 */

#include "display_layout.h"

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

/* External function declarations (avoids cross-directory include path issues) */
extern void BOCompositorClip_Init(int32_t screen_width, int32_t screen_height);
extern void pointer_bounds_init(uint32_t primary_width, uint32_t primary_height);

void DIE_Layout_UpdateSubsystems(DIE_DisplayInfo* info) {
    if (!info) return;

    uint32_t w = info->active_mode.width;
    uint32_t h = info->active_mode.height;
    if (w == 0 || h == 0) return;

    /* 1. Update kernel global geometry authority variables */
    g_kernel_screen_width = w;
    g_kernel_screen_height = h;

    /* 2. Synchronize Pointer Bounds Registry (Input Engine V2 / Pointer Engine V2) */
    pointer_bounds_init(w, h);
    extern void kernel_input_update_resolution(uint32_t w, uint32_t h);
    kernel_input_update_resolution(w, h);

    /* 3. Synchronize BWE Compositor Clip Region */
    BOCompositorClip_Init((int32_t)w, (int32_t)h);
}
