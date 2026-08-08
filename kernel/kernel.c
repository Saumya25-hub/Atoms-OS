#include "kernel/core/core_legacy/boot/include/boot_info.h"
#include "kernel/debug/abde/abde.h"
#include "drivers/interrupt/pic/pic.h"
#include "kernel/core/interrupt/include/irq.h"
#include "kernel/drivers/keyboard/include/keyboard.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
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
extern volatile uint64_t g_irq1_count;

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

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* COM1 Serial UART Output — zero dependency, polled I/O */
void com1_puts(const char *s) {
    while (*s) {
        while ((inb(0x3F8 + 5) & 0x20) == 0);
        outb(0x3F8, *s++);
    }
}

void serial_write_direct(const char *msg) { com1_puts(msg); }
void serial_write_dec_direct(uint64_t val) { (void)val; }
void com1_dbg(const char *msg, ...) { (void)msg; }
void launch_phase_a_proof(void) {}
void launch_phase_b_test1(void) {}
void phase_b_run_test2(void) {}
void phase_b_run_test3(void) {}
void phase_b_test_complete(void) {}

static volatile uint64_t g_timer_ticks = 0;

/* IRQ0 Timer Handler — MINIMAL, NO RENDER, NO DIAG CALLS */
static uint64_t timer_irq_handler(registers_t *regs) {
    (void)regs;
    g_timer_ticks++;
    return 0;
}

void kernel_main(boot_info_t *boot_info) {
    com1_puts("\r\n=== ATOMS OS BOOT TRACE ===\r\n");
    com1_puts("[BOOT] Enter kernel_main\r\n");

    // =====================================================================
    // 1. ABDE Dashboard V2.5 Initialization
    // =====================================================================
    com1_puts("[BOOT] Enter ABDE init\r\n");
    diag_init(boot_info);
    com1_puts("[BOOT] Exit ABDE init\r\n");

    if (boot_info && boot_info->vbe_width > 0 && boot_info->vbe_height > 0) {
        g_kernel_screen_width = boot_info->vbe_width;
        g_kernel_screen_height = boot_info->vbe_height;
    }

    // =====================================================================
    // 2. CPU Features Engine
    // =====================================================================
    com1_puts("[BOOT] Enter CPU\r\n");
    diag_set_running("CPU");
    diag_set_step("CPU FEATURES DETECT");
    cpu_features_init();
    diag_set_pass("CPU");
    diag_set_step("CPU CERTIFIED");
    com1_puts("[BOOT] Exit CPU = PASS\r\n");

    // =====================================================================
    // 3. GDT Engine
    // =====================================================================
    com1_puts("[BOOT] Enter GDT\r\n");
    diag_set_running("GDT");
    diag_set_step("GDT LOAD");
    gdt_init();
    diag_set_pass("GDT");
    diag_set_step("GDT CERTIFIED");
    com1_puts("[BOOT] Exit GDT = PASS\r\n");

    // =====================================================================
    // 4. SMP Engine
    // =====================================================================
    com1_puts("[BOOT] Enter SMP\r\n");
    diag_set_running("SMP");
    diag_set_step("SMP DISCOVERY");
    atoms_smp_discover();
    atoms_smp_initialize_bsp();
    atoms_smp_prepare_aps();
    diag_set_pass("SMP");
    diag_set_step("SMP CERTIFIED");
    com1_puts("[BOOT] Exit SMP = PASS\r\n");

    // =====================================================================
    // 5. IDT Engine
    // =====================================================================
    com1_puts("[BOOT] Enter IDT\r\n");
    diag_set_running("IDT");
    diag_set_step("IDT LOAD");
    idt_init();
    com1_puts("[BOOT] IDT tables loaded\r\n");
    isr_init();
    com1_puts("[BOOT] ISR stubs installed\r\n");
    exception_init();
    com1_puts("[BOOT] Exception handlers armed\r\n");
    diag_set_pass("IDT");
    diag_set_step("IDT CERTIFIED");
    com1_puts("[BOOT] Exit IDT = PASS\r\n");

    // =====================================================================
    // 6. PIC / APIC Engine
    // =====================================================================
    com1_puts("[BOOT] Enter PIC\r\n");
    diag_set_running("PIC");
    diag_set_step("PIC REMAP MASTER/SLAVE");

    com1_puts("[BOOT] PIC pic_init()...\r\n");
    pic_init();
    com1_puts("[BOOT] PIC pic_init() done\r\n");

    com1_puts("[BOOT] PIC irq_init()...\r\n");
    irq_init();
    com1_puts("[BOOT] PIC irq_init() done\r\n");

    com1_puts("[BOOT] PIC irq_register_handler(0)...\r\n");
    irq_register_handler(0, timer_irq_handler);
    com1_puts("[BOOT] PIC irq_register_handler(0) done\r\n");

    com1_puts("[BOOT] PIC keyboard_init()...\r\n");
    keyboard_init();
    com1_puts("[BOOT] PIC keyboard_init() done\r\n");

    com1_puts("[BOOT] PIC pic_clear_mask(0)...\r\n");
    pic_clear_mask(0);
    com1_puts("[BOOT] PIC pic_clear_mask(1)...\r\n");
    pic_clear_mask(1);
    com1_puts("[BOOT] PIC masks cleared\r\n");

    diag_set_step("PIC REMAP DONE");
    diag_set_pic_telemetry(true, true, 0xFEC00000, 0, 0, 0, 0x20);
    diag_set_pass("PIC");
    diag_set_step("PIC CERTIFIED");
    com1_puts("[BOOT] Exit PIC = PASS\r\n");

    // =====================================================================
    // 7. STI — Enable Hardware Interrupts (Timer IRQ0 Starts Firing Here)
    // =====================================================================
    com1_puts("[BOOT] STI: Enabling interrupts NOW\r\n");
    __asm__ volatile("sti");
    com1_puts("[BOOT] STI: Interrupts enabled\r\n");

    // =====================================================================
    // 8. PMM Physical Memory Manager
    // =====================================================================
    com1_puts("[BOOT] Enter PMM\r\n");
    diag_set_running("PMM");
    diag_set_step("PMM INIT START");
    pmm_init(boot_info);
    diag_set_pass("PMM");
    diag_set_step("PMM CERTIFIED");
    com1_puts("[BOOT] Exit PMM = PASS\r\n");

    // =====================================================================
    // 9. VMM Virtual Memory Manager — THE TARGET
    // =====================================================================
    com1_puts("[BOOT] >>>>>>> ENTER VMM <<<<<<< \r\n");
    diag_set_running("VMM");
    diag_set_step("VMM ENTRY");
    com1_puts("[BOOT] VMM diag_set_running done\r\n");

    vmm_init();

    diag_set_pass("VMM");
    diag_set_step("VMM CERTIFIED");
    com1_puts("[BOOT] >>>>>>> EXIT VMM = PASS <<<<<<< \r\n");

    // =====================================================================
    // 10. HEAP — Next Target
    // =====================================================================
    com1_puts("[BOOT] Enter HEAP ready\r\n");
    diag_set_running("HEAP");
    diag_set_step("HEAP INIT READY");

    // =====================================================================
    // ACTIVE HEARTBEAT HALT LOOP — Keeps ABDE alive for photo capture
    // =====================================================================
    com1_puts("[BOOT] Entering heartbeat halt loop\r\n");
    for (;;) {
        diag_heartbeat_tick();
        for (volatile int i = 0; i < 5000000; i++) {
            __asm__ __volatile__("nop");
        }
    }
}
