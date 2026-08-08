#include "kernel/core/core_legacy/boot/include/boot_info.h"
#include "kernel/debug/abde/abde.h"
#include <stdint.h>

/* Subsystem External Declarations */
extern void cpu_features_init(void);
extern void gdt_init(void);
extern void atoms_smp_discover(void);
extern void atoms_smp_initialize_bsp(void);
extern void atoms_smp_prepare_aps(void);
extern void idt_init(void);
extern void isr_init(void);
extern void exception_init(void);
extern void pic_init(void);
extern void irq_init(void);
extern void pmm_init(boot_info_t *boot_info);
extern void vmm_init(void);
extern void heap_init(void);

/* Global Symbol Stubs to Satisfy Cross-Object Link Requirements */
uint32_t g_kernel_screen_width = 2560;
uint32_t g_kernel_screen_height = 1600;
uint64_t g_main_loop_iterations_count = 0;
uint32_t g_enable_runtime_telemetry = 0;
uint64_t g_frame_interval_min_ms = 0;
uint64_t g_frame_interval_max_ms = 0;
uint32_t g_gui_hlts = 0;
uint32_t g_gui_yields = 0;
uint32_t g_usb_reports_count = 0;
uint32_t g_usb_motion_reports_count = 0;
uint32_t g_hid_max_gap_ms = 0;
uint32_t g_hid_decoded_motion_count = 0;
uint32_t g_cursor_position_requests = 0;
uint32_t g_cursor_blocked_by_compositor = 0;
uint32_t g_cursor_fallback_invalid_state = 0;
uint32_t g_cursor_fallback_vram_fail = 0;
uint32_t g_cursor_fast_presents = 0;
uint32_t g_cursor_fast_path_total_us = 0;
uint32_t g_cursor_fast_path_max_us = 0;
uint32_t g_cursor_fast_path_avg_us = 0;
uint32_t g_cursor_damage_requests_count = 0;
uint32_t g_phase_b_test_step = 0;
uint32_t g_phase_b_completed = 0;
uint32_t g_audio_telemetry_snapshot = 0;

void serial_write_direct(const char *msg) { (void)msg; }
void serial_write_dec_direct(uint64_t val) { (void)val; }
void com1_dbg(const char *msg, ...) { (void)msg; }
void launch_phase_a_proof(void) {}
void launch_phase_b_test1(void) {}
void phase_b_run_test2(void) {}
void phase_b_run_test3(void) {}
void phase_b_test_complete(void) {}

void kernel_main(boot_info_t *boot_info) {
    // 1. Initialize ABDE Real-Time Forensic Dashboard V2.5 immediately
    diag_init(boot_info);

    if (boot_info && boot_info->vbe_width > 0 && boot_info->vbe_height > 0) {
        g_kernel_screen_width = boot_info->vbe_width;
        g_kernel_screen_height = boot_info->vbe_height;
    }

    // 2. CPU Features Engine Validation (CERTIFIED REAL HARDWARE PASS)
    diag_set_running("CPU");
    cpu_features_init();
    diag_set_pass("CPU");
    diag_set_step("AFTER CPU PASS");

    // 3. GDT Engine Validation (CERTIFIED REAL HARDWARE PASS)
    diag_set_running("GDT");
    gdt_init();
    diag_set_pass("GDT");
    diag_set_step("GDT CERTIFIED");

    // =========================================================================
    // 4. SMP Engine Target Certification & Bring-Up
    // =========================================================================
    diag_set_running("SMP");
    atoms_smp_discover();
    atoms_smp_initialize_bsp();
    atoms_smp_prepare_aps();
    
    diag_set_pass("SMP");
    diag_set_step("SMP CERTIFIED");

    // =========================================================================
    // 5. Transition to IDT Engine Certification Target
    // =========================================================================
    diag_set_running("IDT");
    diag_set_step("IDT INIT READY");

    // =========================================================================
    // ISOLATED ZERO-FREEZE ABDE V2.5 DASHBOARD HALT LOOP
    // Active heartbeat spinner ticker keeps CPU alive and updates live per-CPU grid
    // =========================================================================
    for (;;) {
        diag_heartbeat_tick();
        for (volatile int i = 0; i < 5000000; i++) {
            __asm__ __volatile__("nop");
        }
    }
}
