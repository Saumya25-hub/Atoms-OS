#include "kernel/graphics/display/validation/include/dve_report.h"

extern void display_print(const char* str);
extern void display_print_dec(uint64_t val);
extern void display_print_hex(uint64_t val);

static void str_copy_safe(char* dst, const char* src, size_t max_len) {
    size_t i = 0;
    while (src && src[i] != '\0' && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void dve_report_init(dve_diagnostic_report_t* report) {
    if (!report) return;
    for (size_t i = 0; i < sizeof(dve_diagnostic_report_t); i++) {
        ((uint8_t*)report)[i] = 0;
    }
    report->confidence = DVE_CONFIDENCE_HIGH;
    report->overall_pass = true;
}

void dve_report_compile(dve_diagnostic_report_t* report) {
    if (!report) return;

    report->overall_pass = (
        report->framebuffer_res.status == DVE_STATUS_PASS &&
        report->geometry_res.status == DVE_STATUS_PASS &&
        report->format_res.status == DVE_STATUS_PASS &&
        report->surface_res.status == DVE_STATUS_PASS &&
        report->driver_res.status == DVE_STATUS_PASS &&
        report->mode_res.status == DVE_STATUS_PASS &&
        report->presentation_res.status == DVE_STATUS_PASS &&
        report->memory_res.status == DVE_STATUS_PASS &&
        report->integrity_res.status == DVE_STATUS_PASS
    );

    if (report->overall_pass) {
        str_copy_safe(report->root_cause_subsystem, "None (All Subsystems Passed Certification)", sizeof(report->root_cause_subsystem));
        report->confidence = DVE_CONFIDENCE_HIGH;
        return;
    }

    /* Prioritized Forensic Root Cause Synthesis Engine */
    if (report->integrity_res.status != DVE_STATUS_PASS || report->integrity_res.is_dead_frame) {
        str_copy_safe(report->root_cause_subsystem, "Frame presented successfully but rendering output is empty / corrupt (Likely failure in compositor, scene generation, clipping, or draw pipeline)", sizeof(report->root_cause_subsystem));
        report->confidence = DVE_CONFIDENCE_HIGH;
    } else if (report->geometry_res.status != DVE_STATUS_PASS) {
        str_copy_safe(report->root_cause_subsystem, "Geometry Engine Pitch/Stride Mismatch between Software Compositor and Hardware Aperture", sizeof(report->root_cause_subsystem));
        report->confidence = DVE_CONFIDENCE_HIGH;
    } else if (report->presentation_res.status != DVE_STATUS_PASS) {
        str_copy_safe(report->root_cause_subsystem, "Presentation Pipeline Copy Size / Hardware Present Flushed Early", sizeof(report->root_cause_subsystem));
        report->confidence = DVE_CONFIDENCE_HIGH;
    } else if (report->framebuffer_res.status != DVE_STATUS_PASS) {
        str_copy_safe(report->root_cause_subsystem, "Framebuffer Memory Base / Page Alignment / VRAM Overflow Failure", sizeof(report->root_cause_subsystem));
        report->confidence = DVE_CONFIDENCE_HIGH;
    } else if (report->driver_res.status != DVE_STATUS_PASS) {
        str_copy_safe(report->root_cause_subsystem, "GPU Hardware Driver Selection / MMIO BAR Mapping Failure", sizeof(report->root_cause_subsystem));
        report->confidence = DVE_CONFIDENCE_HIGH;
    } else if (report->memory_res.status != DVE_STATUS_PASS) {
        str_copy_safe(report->root_cause_subsystem, "Memory Safety Scanline Buffer Overrun / Alignment Failure", sizeof(report->root_cause_subsystem));
        report->confidence = DVE_CONFIDENCE_HIGH;
    } else if (report->format_res.status != DVE_STATUS_PASS) {
        str_copy_safe(report->root_cause_subsystem, "Pixel Format Channel Mask / Alpha Channel Ordering Mismatch", sizeof(report->root_cause_subsystem));
        report->confidence = DVE_CONFIDENCE_MEDIUM;
    } else if (report->surface_res.status != DVE_STATUS_PASS) {
        str_copy_safe(report->root_cause_subsystem, "HAL Surface Descriptor Lifetime / Alignment Violation", sizeof(report->root_cause_subsystem));
        report->confidence = DVE_CONFIDENCE_MEDIUM;
    } else {
        str_copy_safe(report->root_cause_subsystem, "Display Mode Scanout / Refresh Rate Boundary Error", sizeof(report->root_cause_subsystem));
        report->confidence = DVE_CONFIDENCE_LOW;
    }
}

void dve_report_print(const dve_diagnostic_report_t* report) {
    if (!report) return;

    display_print("\n=========================================\n");
    display_print("    DISPLAY VALIDATION ENGINE REPORT    \n");
    display_print("=========================================\n");

    display_print("Framebuffer ........ ");
    display_print(report->framebuffer_res.status == DVE_STATUS_PASS ? "PASS\n" : "FAIL\n");

    display_print("Surface ............ ");
    display_print(report->surface_res.status == DVE_STATUS_PASS ? "PASS\n" : "FAIL\n");

    display_print("Geometry ........... ");
    display_print(report->geometry_res.status == DVE_STATUS_PASS ? "PASS\n" : "FAIL\n");

    display_print("Pixel Format ....... ");
    display_print(report->format_res.status == DVE_STATUS_PASS ? "PASS\n" : "FAIL\n");

    display_print("Display Mode ....... ");
    display_print(report->mode_res.status == DVE_STATUS_PASS ? "PASS\n" : "FAIL\n");

    display_print("GPU Driver ......... ");
    display_print(report->driver_res.status == DVE_STATUS_PASS ? "PASS\n" : "FAIL\n");

    display_print("Presentation ....... ");
    display_print(report->presentation_res.status == DVE_STATUS_PASS ? "PASS\n" : "FAIL\n");

    display_print("Memory Safety ...... ");
    display_print(report->memory_res.status == DVE_STATUS_PASS ? "PASS\n" : "FAIL\n");

    display_print("Frame Integrity .... ");
    display_print(report->integrity_res.status == DVE_STATUS_PASS ? "PASS\n" : "FAIL\n");

    display_print("-----------------------------------------\n");

    display_print("Expected Pitch     : "); display_print_dec(report->geometry_res.expected_pitch); display_print(" bytes\n");
    display_print("Actual Pitch       : "); display_print_dec(report->geometry_res.actual_pitch); display_print(" bytes\n");

    display_print("Expected Width     : "); display_print_dec(report->geometry_res.expected_width); display_print(" px\n");
    display_print("Actual Width       : "); display_print_dec(report->geometry_res.actual_width); display_print(" px\n");

    display_print("Expected Height    : "); display_print_dec(report->geometry_res.expected_height); display_print(" px\n");
    display_print("Actual Height      : "); display_print_dec(report->geometry_res.actual_height); display_print(" px\n");

    display_print("Expected Copy Size : "); display_print_dec(report->presentation_res.expected_copy_size); display_print(" bytes\n");
    display_print("Actual Copy Size   : "); display_print_dec(report->presentation_res.actual_copy_size); display_print(" bytes\n");

    display_print("Frame CRC32        : "); display_print_hex(report->integrity_res.crc32); display_print("\n");
    display_print("Frame Hash (FNV1a) : "); display_print_hex(report->integrity_res.fnv1a_hash); display_print("\n");

    display_print("Detected Image     : ");
    if (report->integrity_res.is_dead_frame) {
        display_print("INVALID (Empty Black Frame)\n\n");
    } else {
        display_print("VALID (Rendered Pixel Content Present)\n\n");
    }

    display_print("Root Cause:\n");
    display_print(report->root_cause_subsystem);
    display_print("\n\nConfidence:\n");

    switch (report->confidence) {
        case DVE_CONFIDENCE_HIGH:   display_print("HIGH\n"); break;
        case DVE_CONFIDENCE_MEDIUM: display_print("MEDIUM\n"); break;
        case DVE_CONFIDENCE_LOW:    display_print("LOW\n"); break;
        default:                    display_print("NONE\n"); break;
    }

    display_print("=========================================\n\n");
}
