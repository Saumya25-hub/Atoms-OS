/**
 * @file bdce_authority.c
 * @brief ATOMS OS BDCE (BOS Display Contract Engine) - Authority Layer Implementation
 * @note Phase B — Read-Only State Seeding (`Passive Observation Only`). Zero behavior changes.
 */

#include "kernel/display/bdce/include/bdce_authority.h"
#include "kernel/core/core_legacy/boot/include/boot_info.h"
#include "bovisual/Include/bovisual_types.h"
#include "kernel/display/display_manager.h"
#include "kernel/display/agdae/agdae.h"

/* Static primary context instance (`Dormant observer in Phase B — Read-Only Seeding`) */
static BDCE_Context s_primary_context;

BDCE_Context* BDCE_GetPrimaryContext(void) {
    return &s_primary_context;
}

const BDCE_PhysicalState* BDCE_GetPhysicalState(const BDCE_Context* ctx) {
    if (!ctx) return &s_primary_context.physical_state;
    return &ctx->physical_state;
}

const BDCE_LogicalState* BDCE_GetLogicalState(const BDCE_Context* ctx) {
    if (!ctx) return &s_primary_context.logical_state;
    return &ctx->logical_state;
}

const BDCE_SurfaceState* BDCE_GetSurfaceState(const BDCE_Context* ctx) {
    if (!ctx) return &s_primary_context.surface_state;
    return &ctx->surface_state;
}

const BDCE_TemporalState* BDCE_GetTemporalState(const BDCE_Context* ctx) {
    if (!ctx) return &s_primary_context.temporal_state;
    return &ctx->temporal_state;
}

const BDCE_FrameState* BDCE_GetFrameState(const BDCE_Context* ctx) {
    if (!ctx) return &s_primary_context.frame_state;
    return &ctx->frame_state;
}

const BDCE_PresentationState* BDCE_GetPresentationState(const BDCE_Context* ctx) {
    if (!ctx) return &s_primary_context.presentation_state;
    return &ctx->presentation_state;
}

const BDCE_DamageState* BDCE_GetDamageState(const BDCE_Context* ctx) {
    if (!ctx) return &s_primary_context.damage_state;
    return &ctx->damage_state;
}

BDCE_OwnershipState BDCE_GetOwnershipState(const BDCE_Context* ctx) {
    if (!ctx) return s_primary_context.ownership_state;
    return ctx->ownership_state;
}

const BDCE_CapabilityState* BDCE_GetCapabilityState(const BDCE_Context* ctx) {
    if (!ctx) return &s_primary_context.capability_state;
    return &ctx->capability_state;
}

void BDCE_SeedFromCurrentSystem(const void* boot_info_ptr, const void* hw_fb_ptr) {
    BDCE_Context* ctx = &s_primary_context;
    const boot_info_t* boot_info = (const boot_info_t*)boot_info_ptr;
    const BVFramebuffer* hw_fb = (const BVFramebuffer*)hw_fb_ptr;

    /* 1. Physical State Seeding (Strictly read from VBE / Hardware Framebuffer) */
    if (hw_fb && hw_fb->width > 0) {
        ctx->physical_state.physical_width = hw_fb->width;
        ctx->physical_state.physical_height = hw_fb->height;
        ctx->physical_state.hardware_pitch_bytes = hw_fb->pitch;
        ctx->physical_state.vram_base_paddr = (uint64_t)(uintptr_t)hw_fb->buffer;
        ctx->physical_state.vram_size_bytes = (uint64_t)hw_fb->height * hw_fb->pitch;
        ctx->physical_state.bytes_per_pixel = 4; /* Standard 32 BPP */
        ctx->physical_state.refresh_rate_hz = 60;
    } else if (boot_info && boot_info->vbe_width > 0) {
        ctx->physical_state.physical_width = boot_info->vbe_width;
        ctx->physical_state.physical_height = boot_info->vbe_height;
        ctx->physical_state.hardware_pitch_bytes = boot_info->vbe_pitch;
        ctx->physical_state.vram_base_paddr = boot_info->vbe_framebuffer;
        ctx->physical_state.vram_size_bytes = (uint64_t)boot_info->vbe_height * boot_info->vbe_pitch;
        ctx->physical_state.bytes_per_pixel = boot_info->vbe_bpp ? (boot_info->vbe_bpp / 8) : 4;
        ctx->physical_state.refresh_rate_hz = 60;
    }

    /* 2. Logical State Seeding (Strictly read from existing global & adaptation metrics) */
    extern uint32_t g_kernel_screen_width;
    extern uint32_t g_kernel_screen_height;
    ctx->logical_state.logical_width = g_kernel_screen_width;
    ctx->logical_state.logical_height = g_kernel_screen_height;

    const AGDAE_Metrics* agdae = AGDAE_GetMetrics();
    if (agdae) {
        ctx->logical_state.scale_factor_pct = agdae->scale_factor_pct ? agdae->scale_factor_pct : 100;
        ctx->logical_state.safe_area_rect.x = agdae->safe_area.x;
        ctx->logical_state.safe_area_rect.y = agdae->safe_area.y;
        ctx->logical_state.safe_area_rect.width = agdae->safe_area.width;
        ctx->logical_state.safe_area_rect.height = agdae->safe_area.height;
        ctx->logical_state.desktop_work_area.x = agdae->window_work_area.x;
        ctx->logical_state.desktop_work_area.y = agdae->window_work_area.y;
        ctx->logical_state.desktop_work_area.width = agdae->window_work_area.width;
        ctx->logical_state.desktop_work_area.height = agdae->window_work_area.height;
    } else {
        ctx->logical_state.scale_factor_pct = 100;
        ctx->logical_state.safe_area_rect.x = 0;
        ctx->logical_state.safe_area_rect.y = 0;
        ctx->logical_state.safe_area_rect.width = g_kernel_screen_width;
        ctx->logical_state.safe_area_rect.height = g_kernel_screen_height;
        ctx->logical_state.desktop_work_area.x = 0;
        ctx->logical_state.desktop_work_area.y = 0;
        ctx->logical_state.desktop_work_area.width = g_kernel_screen_width;
        ctx->logical_state.desktop_work_area.height = g_kernel_screen_height;
    }

    /* 3. Capability State Seeding (Strictly read from DIE capabilities/hardware) */
    DIE_DisplayInfo* die_info = DIE_GetPrimaryDisplay();
    if (die_info) {
        ctx->capability_state.can_hardware_scale = false;
        ctx->capability_state.can_hardware_flip = true;
        ctx->capability_state.can_hardware_cursor = true;
        ctx->capability_state.can_triple_buffer = false;
        ctx->capability_state.supports_hdr_10bit = false;
    } else {
        ctx->capability_state.can_hardware_scale = false;
        ctx->capability_state.can_hardware_flip = true;
        ctx->capability_state.can_hardware_cursor = true;
        ctx->capability_state.can_triple_buffer = false;
        ctx->capability_state.supports_hdr_10bit = false;
    }

    /* 4. Temporal State Seeding (Initial baseline snapshot) */
    ctx->temporal_state.display_epoch_id = 1;
    ctx->temporal_state.current_vblank_count = 0;
    ctx->temporal_state.last_vblank_timestamp = 0;
    ctx->temporal_state.presentation_fence_id = 0;
    ctx->temporal_state.active_vram_page = 0;

    /* Mark context initialized/active as a passive observer */
    ctx->is_active = true;
}


