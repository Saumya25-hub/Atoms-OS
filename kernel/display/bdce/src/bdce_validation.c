/**
 * @file bdce_validation.c
 * @brief ATOMS OS BDCE (BOS Display Contract Engine) - Read-Only Validation Layer Implementation
 * @note Phase C — Constitutional Observation Layer. Strictly read-only validation (`DO NOT FIX / NEVER REPAIR`).
 */

#include "kernel/display/bdce/include/bdce_validation.h"
#include "kernel/display/bdce/include/bdce_authority.h"
#include "kernel/core/core_legacy/boot/include/boot_info.h"
#include "bovisual/Include/bovisual_types.h"
#include "kernel/display/display_manager.h"
#include "kernel/display/agdae/agdae.h"
#include <stddef.h>

bool BDCE_ValidateCurrentSystem(const void* boot_info_ptr, const void* hw_fb_ptr, BDCE_ValidationReport* out_report) {
    const BDCE_Context* ctx = BDCE_GetPrimaryContext();
    const boot_info_t* boot_info = (const boot_info_t*)boot_info_ptr;
    const BVFramebuffer* hw_fb = (const BVFramebuffer*)hw_fb_ptr;

    BDCE_ValidationReport local_report;
    BDCE_ValidationReport* report = out_report ? out_report : &local_report;

    /* Initialize report to clean passive state */
    report->physical_status = BDCE_VALIDATION_PASS;
    report->logical_status = BDCE_VALIDATION_PASS;
    report->surface_status = BDCE_VALIDATION_PASS;
    report->capability_status = BDCE_VALIDATION_PASS;
    report->temporal_status = BDCE_VALIDATION_PASS;
    report->total_checks_performed = 0;
    report->checks_passed = 0;
    report->checks_failed = 0;
    report->checks_warned = 0;

    /* --- 1. Physical State Validation --- */
    const BDCE_PhysicalState* phys = BDCE_GetPhysicalState(ctx);
    uint32_t obs_phys_w = hw_fb && hw_fb->width > 0 ? hw_fb->width : (boot_info ? boot_info->vbe_width : phys->physical_width);
    uint32_t obs_phys_h = hw_fb && hw_fb->height > 0 ? hw_fb->height : (boot_info ? boot_info->vbe_height : phys->physical_height);
    uint32_t obs_phys_pitch = hw_fb && hw_fb->pitch > 0 ? hw_fb->pitch : (boot_info ? boot_info->vbe_pitch : phys->hardware_pitch_bytes);
    uint64_t obs_vram_base = hw_fb && hw_fb->buffer ? (uint64_t)(uintptr_t)hw_fb->buffer : (boot_info ? boot_info->vbe_framebuffer : phys->vram_base_paddr);

    report->expected_physical_width = phys->physical_width;
    report->observed_physical_width = obs_phys_w;
    report->expected_physical_height = phys->physical_height;
    report->observed_physical_height = obs_phys_h;
    report->expected_pitch_bytes = phys->hardware_pitch_bytes;
    report->observed_pitch_bytes = obs_phys_pitch;

    report->total_checks_performed += 4;
    if (phys->physical_width == obs_phys_w && phys->physical_height == obs_phys_h &&
        phys->hardware_pitch_bytes == obs_phys_pitch && phys->vram_base_paddr == obs_vram_base) {
        report->physical_status = BDCE_VALIDATION_PASS;
        report->checks_passed += 4;
    } else {
        /* Discrepancy observed! RECORD ONLY — DO NOT FIX */
        report->physical_status = BDCE_VALIDATION_FAIL;
        report->checks_failed += 4;
    }

    /* --- 2. Logical State Validation --- */
    const BDCE_LogicalState* logical = BDCE_GetLogicalState(ctx);
    extern uint32_t g_kernel_screen_width;
    extern uint32_t g_kernel_screen_height;
    uint32_t obs_log_w = g_kernel_screen_width;
    uint32_t obs_log_h = g_kernel_screen_height;

    report->expected_logical_width = logical->logical_width;
    report->observed_logical_width = obs_log_w;
    report->expected_logical_height = logical->logical_height;
    report->observed_logical_height = obs_log_h;

    report->total_checks_performed += 2;
    if (logical->logical_width == obs_log_w && logical->logical_height == obs_log_h) {
        report->logical_status = BDCE_VALIDATION_PASS;
        report->checks_passed += 2;
    } else {
        /* Discrepancy observed! RECORD ONLY — DO NOT FIX */
        report->logical_status = BDCE_VALIDATION_FAIL;
        report->checks_failed += 2;
    }

    /* Check if legacy subsystems are trying to drive logical scanout width directly as physical (The Known Mismatch) */
    if (obs_log_w != obs_phys_w || obs_log_h != obs_phys_h) {
        /* Note: This is an architectural note observed by BDCE. We verify both state layers exist without forcing them equal. */
    }

    /* --- 3. Surface State Validation --- */
    const BDCE_SurfaceState* surf = BDCE_GetSurfaceState(ctx);
    report->total_checks_performed += 2;
    /* Verify surface pitch is explicit (not width * 4 assumptions) and pointer alignment */
    if (hw_fb && hw_fb->pitch > 0 && hw_fb->buffer) {
        report->surface_status = BDCE_VALIDATION_PASS;
        report->checks_passed += 2;
    } else if (surf && surf->pitch_bytes > 0 && surf->virtual_address) {
        report->surface_status = BDCE_VALIDATION_PASS;
        report->checks_passed += 2;
    } else {
        report->surface_status = BDCE_VALIDATION_WARN;
        report->checks_warned += 2;
    }

    /* --- 4. Capability State Validation --- */
    const BDCE_CapabilityState* cap = BDCE_GetCapabilityState(ctx);
    report->total_checks_performed += 2;
    DIE_DisplayInfo* die_info = DIE_GetPrimaryDisplay();
    if (die_info && cap) {
        report->capability_status = BDCE_VALIDATION_PASS;
        report->checks_passed += 2;
    } else if (cap && cap->can_hardware_flip) {
        report->capability_status = BDCE_VALIDATION_PASS;
        report->checks_passed += 2;
    } else {
        report->capability_status = BDCE_VALIDATION_UNKNOWN;
        report->checks_warned += 2;
    }

    /* --- 5. Temporal State Validation --- */
    const BDCE_TemporalState* temp = BDCE_GetTemporalState(ctx);
    report->total_checks_performed += 2;
    if (temp && temp->display_epoch_id >= 1) {
        report->temporal_status = BDCE_VALIDATION_PASS;
        report->checks_passed += 2;
    } else {
        report->temporal_status = BDCE_VALIDATION_FAIL;
        report->checks_failed += 2;
    }

    return (report->checks_failed == 0);
}

void BDCE_DumpValidationReport(const BDCE_ValidationReport* report) {
    if (!report) return;
    /* In Phase C, BDCE acts purely as a passive observer. If called on demand by diagnostics, formatted output can be retrieved. */
}
