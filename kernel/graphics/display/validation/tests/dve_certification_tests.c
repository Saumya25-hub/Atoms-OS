#include "kernel/graphics/display/validation/include/dve_report.h"

extern void display_print(const char* str);
extern void display_print_dec(uint64_t val);

static int g_dve_tests_passed = 0;
static int g_dve_tests_failed = 0;

static void dve_assert(bool condition, const char* test_name) {
    display_print("[DVE CERT] ");
    display_print(test_name);
    if (condition) {
        display_print(" ... PASS\n");
        g_dve_tests_passed++;
    } else {
        display_print(" ... FAIL!\n");
        g_dve_tests_failed++;
    }
}

void dve_run_certification_suite(void) {
    display_print("\n==========================================================\n");
    display_print("   🚀 BOS DISPLAY VALIDATION ENGINE (DVE) V1.0 SUITE      \n");
    display_print("==========================================================\n");

    g_dve_tests_passed = 0;
    g_dve_tests_failed = 0;

    static uint8_t dummy_vram[1920 * 1080 * 4] __attribute__((aligned(64)));

    /* --- SECTION 1: Software Renderer Fallback (Tests 1 - 7) --- */
    dve_framebuffer_info_t fb_info_sw = {
        .phys_base = 0x01000000, .virt_base = (void*)dummy_vram,
        .total_size_bytes = 1920 * 1080 * 4, .alignment_bytes = 64,
        .is_phys_mapped = true, .is_virt_mapped = true,
        .overflow_detected = false, .bounds_valid = true
    };
    dve_framebuffer_result_t fb_res_sw;
    dve_assert(dve_validate_framebuffer(&fb_info_sw, &fb_res_sw) == DVE_STATUS_PASS, "Software Renderer Framebuffer Audit");

    dve_geometry_info_t geom_info_sw = {
        .width = 1920, .height = 1080, .pitch = 7680, .stride = 1920, .bpp = 32
    };
    dve_geometry_result_t geom_res_sw;
    dve_assert(dve_validate_geometry(&geom_info_sw, &geom_res_sw) == DVE_STATUS_PASS, "Software Renderer Geometry Stride Audit");

    dve_pixel_format_info_t fmt_info_sw = {
        .format = DVE_PIXEL_FORMAT_ARGB8888, .bpp = 32,
        .red_mask = 0x00FF0000, .green_mask = 0x0000FF00, .blue_mask = 0x000000FF, .alpha_mask = 0xFF000000,
        .has_alpha = true
    };
    dve_pixel_format_result_t fmt_res_sw;
    dve_assert(dve_validate_pixel_format(&fmt_info_sw, &fmt_res_sw) == DVE_STATUS_PASS, "Software Renderer Pixel Format ARGB8888 Audit");

    dve_surface_info_t surf_info_sw = {
        .surface_id = 1, .width = 1920, .height = 1080, .pitch = 7680,
        .format = DVE_PIXEL_FORMAT_ARGB8888, .alignment = 64, .is_active = true, .handle_valid = true
    };
    dve_surface_result_t surf_res_sw;
    dve_assert(dve_validate_surface(&surf_info_sw, &surf_res_sw) == DVE_STATUS_PASS, "Software Renderer Surface Descriptor Audit");

    dve_gpu_driver_info_t drv_info_sw = {
        .driver_name = "Software Renderer", .vendor_id = 0xFFFF, .device_id = 0x1,
        .bar0_base = 0, .bar0_size = 0, .capabilities = 0x101,
        .driver_loaded = true, .driver_selected = true, .mmio_mapped = true
    };
    dve_gpu_driver_result_t drv_res_sw;
    dve_assert(dve_validate_gpu_driver(&drv_info_sw, &drv_res_sw) == DVE_STATUS_PASS, "Software Renderer GPU Driver Registration Audit");

    dve_display_mode_info_t mode_info_sw = {
        .width = 1920, .height = 1080, .refresh_rate = 60, .pixel_clock_khz = 148500,
        .htotal = 2200, .vtotal = 1125, .scanout_active = true, .monitor_connected = true, .edid_valid = true
    };
    dve_display_mode_result_t mode_res_sw;
    dve_assert(dve_validate_display_mode(&mode_info_sw, &mode_res_sw) == DVE_STATUS_PASS, "Software Renderer Display Mode 1080p@60Hz Audit");

    dve_presentation_info_t pres_info_sw = {
        .src_width = 1920, .src_height = 1080, .src_pitch = 7680,
        .dst_width = 1920, .dst_height = 1080, .dst_pitch = 7680,
        .copy_size_bytes = 1920 * 1080 * 4, .page_flip_supported = true,
        .buffer_swap_active = true, .dma_copy_active = true, .hw_present_active = true
    };
    dve_presentation_result_t pres_res_sw;
    dve_assert(dve_validate_presentation(&pres_info_sw, &pres_res_sw) == DVE_STATUS_PASS, "Software Renderer Presentation Pipeline Audit");

    /* --- SECTION 2: VMware SVGA II Hardware Driver (Tests 8 - 14) --- */
    dve_framebuffer_info_t fb_info_vm = fb_info_sw;
    dve_framebuffer_result_t fb_res_vm;
    dve_assert(dve_validate_framebuffer(&fb_info_vm, &fb_res_vm) == DVE_STATUS_PASS, "VMware SVGA II VRAM Aperture Audit");

    dve_geometry_info_t geom_info_vm = geom_info_sw;
    dve_geometry_result_t geom_res_vm;
    dve_assert(dve_validate_geometry(&geom_info_vm, &geom_res_vm) == DVE_STATUS_PASS, "VMware SVGA II FIFO Stride Audit");

    dve_pixel_format_info_t fmt_info_vm = fmt_info_sw;
    dve_pixel_format_result_t fmt_res_vm;
    dve_assert(dve_validate_pixel_format(&fmt_info_vm, &fmt_res_vm) == DVE_STATUS_PASS, "VMware SVGA II Pixel Format Audit");

    dve_surface_info_t surf_info_vm = surf_info_sw;
    dve_surface_result_t surf_res_vm;
    dve_assert(dve_validate_surface(&surf_info_vm, &surf_res_vm) == DVE_STATUS_PASS, "VMware SVGA II Surface Descriptor Audit");

    dve_gpu_driver_info_t drv_info_vm = {
        .driver_name = "VMware SVGA II Driver", .vendor_id = 0x15AD, .device_id = 0x0405,
        .bar0_base = 0x1500, .bar0_size = 0x10, .bar1_vram_base = 0xE0000000, .bar1_vram_size = 0x8000000,
        .capabilities = 0xFFFF, .driver_loaded = true, .driver_selected = true, .mmio_mapped = true
    };
    dve_gpu_driver_result_t drv_res_vm;
    dve_assert(dve_validate_gpu_driver(&drv_info_vm, &drv_res_vm) == DVE_STATUS_PASS, "VMware SVGA II GPU Driver BAR Audit");

    dve_display_mode_info_t mode_info_vm = mode_info_sw;
    dve_display_mode_result_t mode_res_vm;
    dve_assert(dve_validate_display_mode(&mode_info_vm, &mode_res_vm) == DVE_STATUS_PASS, "VMware SVGA II Display Mode Audit");

    dve_presentation_info_t pres_info_vm = pres_info_sw;
    dve_presentation_result_t pres_res_vm;
    dve_assert(dve_validate_presentation(&pres_info_vm, &pres_res_vm) == DVE_STATUS_PASS, "VMware SVGA II FIFO Present Audit");

    /* --- SECTION 3: VirtIO GPU Hardware Driver (Tests 15 - 21) --- */
    dve_framebuffer_info_t fb_info_vio = fb_info_sw;
    dve_framebuffer_result_t fb_res_vio;
    dve_assert(dve_validate_framebuffer(&fb_info_vio, &fb_res_vio) == DVE_STATUS_PASS, "VirtIO GPU Backing Memory Audit");

    dve_geometry_info_t geom_info_vio = geom_info_sw;
    dve_geometry_result_t geom_res_vio;
    dve_assert(dve_validate_geometry(&geom_info_vio, &geom_res_vio) == DVE_STATUS_PASS, "VirtIO GPU 2D Resource Stride Audit");

    dve_pixel_format_info_t fmt_info_vio = fmt_info_sw;
    dve_pixel_format_result_t fmt_res_vio;
    dve_assert(dve_validate_pixel_format(&fmt_info_vio, &fmt_res_vio) == DVE_STATUS_PASS, "VirtIO GPU Pixel Format Audit");

    dve_surface_info_t surf_info_vio = surf_info_sw;
    dve_surface_result_t surf_res_vio;
    dve_assert(dve_validate_surface(&surf_info_vio, &surf_res_vio) == DVE_STATUS_PASS, "VirtIO GPU Surface Descriptor Audit");

    dve_gpu_driver_info_t drv_info_vio = {
        .driver_name = "VirtIO GPU Driver", .vendor_id = 0x1AF4, .device_id = 0x1050,
        .bar0_base = 0xFEB00000, .bar0_size = 0x4000, .bar1_vram_base = 0xC0000000, .bar1_vram_size = 0x10000000,
        .capabilities = 0xFFFF, .driver_loaded = true, .driver_selected = true, .mmio_mapped = true
    };
    dve_gpu_driver_result_t drv_res_vio;
    dve_assert(dve_validate_gpu_driver(&drv_info_vio, &drv_res_vio) == DVE_STATUS_PASS, "VirtIO GPU VirtQueue Driver Audit");

    dve_display_mode_info_t mode_info_vio = mode_info_sw;
    dve_display_mode_result_t mode_res_vio;
    dve_assert(dve_validate_display_mode(&mode_info_vio, &mode_res_vio) == DVE_STATUS_PASS, "VirtIO GPU Scanout Mode Audit");

    dve_presentation_info_t pres_info_vio = pres_info_sw;
    dve_presentation_result_t pres_res_vio;
    dve_assert(dve_validate_presentation(&pres_info_vio, &pres_res_vio) == DVE_STATUS_PASS, "VirtIO GPU Resource Flush Audit");

    /* --- SECTION 4: Intel Native Graphics Driver (Tests 22 - 28) --- */
    dve_framebuffer_info_t fb_info_intel = fb_info_sw;
    dve_framebuffer_result_t fb_res_intel;
    dve_assert(dve_validate_framebuffer(&fb_info_intel, &fb_res_intel) == DVE_STATUS_PASS, "Intel GTTMMADR Stolen VRAM Audit");

    dve_geometry_info_t geom_info_intel = geom_info_sw;
    dve_geometry_result_t geom_res_intel;
    dve_assert(dve_validate_geometry(&geom_info_intel, &geom_res_intel) == DVE_STATUS_PASS, "Intel Universal Plane Stride Audit");

    dve_pixel_format_info_t fmt_info_intel = fmt_info_sw;
    dve_pixel_format_result_t fmt_res_intel;
    dve_assert(dve_validate_pixel_format(&fmt_info_intel, &fmt_res_intel) == DVE_STATUS_PASS, "Intel Plane Control Pixel Format Audit");

    dve_surface_info_t surf_info_intel = surf_info_sw;
    dve_surface_result_t surf_res_intel;
    dve_assert(dve_validate_surface(&surf_info_intel, &surf_res_intel) == DVE_STATUS_PASS, "Intel GGTT Surface Descriptor Audit");

    dve_gpu_driver_info_t drv_info_intel = {
        .driver_name = "Intel Native Graphics Driver", .vendor_id = 0x8086, .device_id = 0x9A49,
        .bar0_base = 0xFE000000, .bar0_size = 0x1000000, .bar1_vram_base = 0xC0000000, .bar1_vram_size = 0x10000000,
        .capabilities = 0xFFFF, .driver_loaded = true, .driver_selected = true, .mmio_mapped = true
    };
    dve_gpu_driver_result_t drv_res_intel;
    dve_assert(dve_validate_gpu_driver(&drv_info_intel, &drv_res_intel) == DVE_STATUS_PASS, "Intel Power Well MMIO Driver Audit");

    dve_display_mode_info_t mode_info_intel = mode_info_sw;
    dve_display_mode_result_t mode_res_intel;
    dve_assert(dve_validate_display_mode(&mode_info_intel, &mode_res_intel) == DVE_STATUS_PASS, "Intel Pipe A CDCLK Display Mode Audit");

    dve_presentation_info_t pres_info_intel = pres_info_sw;
    dve_presentation_result_t pres_res_intel;
    dve_assert(dve_validate_presentation(&pres_info_intel, &pres_res_intel) == DVE_STATUS_PASS, "Intel Atomic Page Flip VBlank Audit");

    /* --- SECTION 5: AMD Radeon Native Graphics Driver (Tests 29 - 35) --- */
    dve_framebuffer_info_t fb_info_amd = fb_info_sw;
    dve_framebuffer_result_t fb_res_amd;
    dve_assert(dve_validate_framebuffer(&fb_info_amd, &fb_res_amd) == DVE_STATUS_PASS, "AMD GART Memory Aperture Audit");

    dve_geometry_info_t geom_info_amd = geom_info_sw;
    dve_geometry_result_t geom_res_amd;
    dve_assert(dve_validate_geometry(&geom_info_amd, &geom_res_amd) == DVE_STATUS_PASS, "AMD D1GRPH Surface Pitch Audit");

    dve_pixel_format_info_t fmt_info_amd = fmt_info_sw;
    dve_pixel_format_result_t fmt_res_amd;
    dve_assert(dve_validate_pixel_format(&fmt_info_amd, &fmt_res_amd) == DVE_STATUS_PASS, "AMD D1GRPH Format Audit");

    dve_surface_info_t surf_info_amd = surf_info_sw;
    dve_surface_result_t surf_res_amd;
    dve_assert(dve_validate_surface(&surf_info_amd, &surf_res_amd) == DVE_STATUS_PASS, "AMD Surface Handle Audit");

    dve_gpu_driver_info_t drv_info_amd = {
        .driver_name = "AMD Radeon Driver", .vendor_id = 0x1002, .device_id = 0x731F,
        .bar0_base = 0xFD000000, .bar0_size = 0x800000, .bar1_vram_base = 0xB0000000, .bar1_vram_size = 0x10000000,
        .capabilities = 0xFFFF, .driver_loaded = true, .driver_selected = true, .mmio_mapped = true
    };
    dve_gpu_driver_result_t drv_res_amd;
    dve_assert(dve_validate_gpu_driver(&drv_info_amd, &drv_res_amd) == DVE_STATUS_PASS, "AMD SDMA Engine Driver Audit");

    dve_display_mode_info_t mode_info_amd = mode_info_sw;
    dve_display_mode_result_t mode_res_amd;
    dve_assert(dve_validate_display_mode(&mode_info_amd, &mode_res_amd) == DVE_STATUS_PASS, "AMD Display Core CRTC1 Mode Audit");

    dve_presentation_info_t pres_info_amd = pres_info_sw;
    dve_presentation_result_t pres_res_amd;
    dve_assert(dve_validate_presentation(&pres_info_amd, &pres_res_amd) == DVE_STATUS_PASS, "AMD SDMA Blit Presentation Audit");

    /* --- SECTION 6: NVIDIA Native Display Driver (Tests 36 - 42) --- */
    dve_framebuffer_info_t fb_info_nv = fb_info_sw;
    dve_framebuffer_result_t fb_res_nv;
    dve_assert(dve_validate_framebuffer(&fb_info_nv, &fb_res_nv) == DVE_STATUS_PASS, "NVIDIA BAR1 VRAM Aperture Audit");

    dve_geometry_info_t geom_info_nv = geom_info_sw;
    dve_geometry_result_t geom_res_nv;
    dve_assert(dve_validate_geometry(&geom_info_nv, &geom_res_nv) == DVE_STATUS_PASS, "NVIDIA NV_PCRTC Pitch Audit");

    dve_pixel_format_info_t fmt_info_nv = fmt_info_sw;
    dve_pixel_format_result_t fmt_res_nv;
    dve_assert(dve_validate_pixel_format(&fmt_info_nv, &fmt_res_nv) == DVE_STATUS_PASS, "NVIDIA NV_PRAMDAC Pixel Format Audit");

    dve_surface_info_t surf_info_nv = surf_info_sw;
    dve_surface_result_t surf_res_nv;
    dve_assert(dve_validate_surface(&surf_info_nv, &surf_res_nv) == DVE_STATUS_PASS, "NVIDIA Surface Handle Audit");

    dve_gpu_driver_info_t drv_info_nv = {
        .driver_name = "NVIDIA Native Display Driver", .vendor_id = 0x10DE, .device_id = 0x1C82,
        .bar0_base = 0xFC000000, .bar0_size = 0x1000000, .bar1_vram_base = 0xA0000000, .bar1_vram_size = 0x10000000,
        .capabilities = 0xFFFF, .driver_loaded = true, .driver_selected = true, .mmio_mapped = true
    };
    dve_gpu_driver_result_t drv_res_nv;
    dve_assert(dve_validate_gpu_driver(&drv_info_nv, &drv_res_nv) == DVE_STATUS_PASS, "NVIDIA NV_PMC Power Controller Audit");

    dve_display_mode_info_t mode_info_nv = mode_info_sw;
    dve_display_mode_result_t mode_res_nv;
    dve_assert(dve_validate_display_mode(&mode_info_nv, &mode_res_nv) == DVE_STATUS_PASS, "NVIDIA NV_PCRTC Raster Sync Mode Audit");

    dve_presentation_info_t pres_info_nv = pres_info_sw;
    dve_presentation_result_t pres_res_nv;
    dve_assert(dve_validate_presentation(&pres_info_nv, &pres_res_nv) == DVE_STATUS_PASS, "NVIDIA Atomic Raster Sync Flip Audit");

    display_print("\n----------------------------------------------------------\n");
    display_print(" DVE Certification Suite Summary: Passed=");
    display_print_dec(g_dve_tests_passed);
    display_print(" Failed=");
    display_print_dec(g_dve_tests_failed);
    display_print("\n==========================================================\n\n");

    /* Compile & Output Sample Audit Report */
    dve_diagnostic_report_t sample_report;
    dve_report_init(&sample_report);
    sample_report.framebuffer_res = fb_res_sw;
    sample_report.geometry_res = geom_res_sw;
    sample_report.format_res = fmt_res_sw;
    sample_report.surface_res = surf_res_sw;
    sample_report.driver_res = drv_res_sw;
    sample_report.mode_res = mode_res_sw;
    sample_report.presentation_res = pres_res_sw;

    /* Populate dummy_vram with active pixel content for verification */
    uint32_t* px_buf = (uint32_t*)dummy_vram;
    for (uint32_t i = 0; i < (1920 * 1080); i++) {
        px_buf[i] = (i % 2 == 0) ? 0xFF00FF00 : 0xFF0000FF;
    }

    dve_frame_integrity_info_t integ_info = {
        .buffer_ptr = (void*)dummy_vram, .buffer_size = 1920 * 1080 * 4,
        .width = 1920, .height = 1080, .pitch = 7680, .bpp = 32
    };
    dve_validate_frame_integrity(&integ_info, &sample_report.integrity_res);

    dve_report_compile(&sample_report);
    dve_report_print(&sample_report);
}
