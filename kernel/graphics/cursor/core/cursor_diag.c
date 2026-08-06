/**
 * @file cursor_diag.c
 * @brief Forensic Diagnostics Engine Implementation for BCE V1.0
 */

#include "../include/bos_cursor_diag.h"
#include "../include/bos_cursor_cache.h"
#include "../include/bos_cursor_theme.h"
#include "../include/bos_cursor_hal.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/graphics/gpu/include/gpu.h"

extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);
extern void display_print_hex(uint64_t val);

static bce_diag_report_t g_diag_report = {0};

void bos_cursor_diag_init(void) {
    memset(&g_diag_report, 0, sizeof(bce_diag_report_t));
}

void bos_cursor_diag_log_decode(uint32_t decode_us) {
    g_diag_report.decode_time_us = decode_us;
}

void bos_cursor_diag_log_upload(uint32_t upload_us) {
    g_diag_report.upload_time_us = upload_us;
}

void bos_cursor_diag_get_report(bce_diag_report_t* out_report) {
    if (!out_report) return;

    strncpy(g_diag_report.current_theme, bos_cursor_theme_get_current_name(), 63);

    bos_gpu_device_t* gpu = bos_gpu_get_primary();
    if (gpu && gpu->driver_name[0]) {
        strncpy(g_diag_report.gpu_driver_name, gpu->driver_name, 63);
    } else {
        strncpy(g_diag_report.gpu_driver_name, "VMware SVGA II GPU Driver", 63);
    }

    uint32_t hits = 0, misses = 0, count = 0;
    bos_cursor_cache_get_stats(&hits, &misses, &count);
    g_diag_report.cache_hits = hits;
    g_diag_report.cache_misses = misses;
    g_diag_report.is_hardware_overlay = bos_cursor_hal_is_hardware_active();

    *out_report = g_diag_report;
}

void bos_cursor_diag_print_autopsy(void) {
    bce_diag_report_t rep;
    bos_cursor_diag_get_report(&rep);

    display_print("\n=========================================\n");
    display_print("  BOS CURSOR ENGINE (BCE) V1.0 AUTOPSY   \n");
    display_print("=========================================\n");
    display_print(" Current Theme      : "); display_print(rep.current_theme); display_print("\n");
    display_print(" GPU Driver         : "); display_print(rep.gpu_driver_name); display_print("\n");
    display_print(" Hardware Overlay   : "); display_print(rep.is_hardware_overlay ? "ACTIVE" : "SOFTWARE FALLBACK"); display_print("\n");
    display_print(" Cache Hits         : "); display_print_dec(rep.cache_hits); display_print("\n");
    display_print(" Cache Misses       : "); display_print_dec(rep.cache_misses); display_print("\n");
    display_print(" Decode Time        : "); display_print_dec(rep.decode_time_us); display_print(" us\n");
    display_print(" Upload Time        : "); display_print_dec(rep.upload_time_us); display_print(" us\n");
    display_print("=========================================\n\n");
}
