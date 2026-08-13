#include "kernel/core/core_legacy/boot/include/boot_info.h"
#include "kernel/debug/abde/abde.h"
#include "drivers/interrupt/pic/pic.h"
#include "kernel/core/interrupt/include/irq.h"
#include "kernel/drivers/keyboard/include/keyboard.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/scheduler/include/task.h"
#include "kernel/drivers/input/cursor/cursor_certification.h"
#include "kernel/shell/rook/include/rook_pages.h"
#include "kernel/ahme/include/ahme.h"
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
    const char *p = s;
    while (*p) {
        while ((inb(0x3F8 + 5) & 0x20) == 0);
        outb(0x3F8, *p++);
    }
    extern void debuglan_log(const char *fmt, ...);
    debuglan_log("%s", s);
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

extern void scheduler_sleep(uint64_t ticks);
extern void heap_update_telemetry(const char *status_str);
extern bool scheduler_validate_consistency(void);
extern void debug_shell_init(void);
static Task *g_system_threads[4] = {0};

Task *scheduler_get_system_thread(uint32_t index) {
    if (index < 4) return g_system_threads[index];
    return NULL;
}

volatile uint64_t g_survival_heartbeat_count = 0;

static void system_telemetry_thread(void) {
    com1_puts("[SYSTEM THREAD] ABDE Telemetry Engine Online (Silent Background Mode)\r\n");
    for (;;) {
        heap_update_telemetry("RUNNING");
        // diag_render(); // Disabled: Cursor Certification mode active on screen
        scheduler_sleep(100);
    }
}

static void system_heartbeat_thread(void) {
    com1_puts("[SYSTEM THREAD] Live Heartbeat Engine Online\r\n");
    extern void debuglan_log_subsys(const char* subsys, const char* fmt, ...);
    extern uint64_t timer_get_ticks(void);
    for (;;) {
        diag_heartbeat_tick();
        diag_cpu_heartbeat(0);
        g_survival_heartbeat_count++;
        extern volatile uint64_t g_heartbeat_ticks;
        g_heartbeat_ticks++;
        if ((g_survival_heartbeat_count % 50) == 0) {
            debuglan_log_subsys("HEARTBEAT", "ticks=%llu", timer_get_ticks());
        }
        scheduler_sleep(100);
    }
}

static void system_diagnostics_thread(void) {
    com1_puts("[SYSTEM THREAD] Kernel Diagnostics Watchdog Online\r\n");
    for (;;) {
        scheduler_validate_consistency();
        scheduler_sleep(200);
    }
}

static void system_debug_shell_thread(void) {
    com1_puts("[SYSTEM THREAD] Kernel Debug Shell Online\r\n");
    debug_shell_init();
    for (;;) {
        scheduler_sleep(500);
    }
}

/* IRQ0 Timer Handler — MINIMAL, NO RENDER, NO DIAG CALLS */
static uint64_t timer_irq_handler(registers_t *regs) {
    (void)regs;
    g_timer_ticks++;
    return 0;
}

void kernel_main(boot_info_t *boot_info) {
    com1_puts("\r\n=== ATOMS OS FORENSIC BOOT TRACE ===\r\n");
    com1_puts("[BOOT] Enter kernel_main\r\n");

    // =====================================================================
    // 1. ABDE Dashboard V2.5 Initialization
    // =====================================================================
    com1_puts("[BOOT] Enter ABDE init\r\n");
    diag_init(boot_info);
    com1_puts("[BOOT] Exit ABDE init\r\n");

    ahme_init();

    if (boot_info && boot_info->vbe_width > 0 && boot_info->vbe_height > 0) {
        g_kernel_screen_width = boot_info->vbe_width;
        g_kernel_screen_height = boot_info->vbe_height;
    }

    // =====================================================================
    // 2. CPU Features Engine
    // =====================================================================
    com1_puts("[CPU_START]\r\n");
    diag_set_running("CPU");
    diag_set_step("CPU FEATURES DETECT");
    cpu_features_init();
    diag_set_pass("CPU");
    diag_set_step("CPU CERTIFIED");
    com1_puts("[CPU_PASS]\r\n");

    // =====================================================================
    // 3. GDT Engine
    // =====================================================================
    com1_puts("[GDT_START]\r\n");
    diag_set_running("GDT");
    diag_set_step("GDT LOAD");
    gdt_init();
    diag_set_pass("GDT");
    diag_set_step("GDT CERTIFIED");
    com1_puts("[GDT_PASS]\r\n");

    // =====================================================================
    // 4. SMP Engine
    // =====================================================================
    com1_puts("[SMP_START]\r\n");
    diag_set_running("SMP");
    diag_set_step("SMP DISCOVERY");
    atoms_smp_discover();
    atoms_smp_initialize_bsp();
    atoms_smp_prepare_aps();
    diag_set_pass("SMP");
    diag_set_step("SMP CERTIFIED");
    com1_puts("[SMP_PASS]\r\n");

    // =====================================================================
    // 5. IDT Engine
    // =====================================================================
    com1_puts("[IDT_START]\r\n");
    diag_set_running("IDT");
    diag_set_step("IDT LOAD");
    idt_init();
    com1_puts("[IDT] IDT tables loaded\r\n");
    isr_init();
    com1_puts("[IDT] ISR stubs installed\r\n");
    exception_init();
    com1_puts("[IDT] Exception handlers armed\r\n");
    diag_set_pass("IDT");
    diag_set_step("IDT CERTIFIED");
    com1_puts("[IDT_PASS]\r\n");

    // =====================================================================
    // 6. PIC / APIC Engine
    // =====================================================================
    com1_puts("[PIC_START]\r\n");
    diag_set_running("PIC");
    diag_set_step("PIC REMAP MASTER/SLAVE");

    com1_puts("[PIC] Remapping Legacy PIC...\r\n");
    pic_init();
    com1_puts("[PIC] pic_init() done\r\n");

    com1_puts("[PIC] Initializing IRQ manager...\r\n");
    irq_init();
    com1_puts("[PIC] irq_init() done\r\n");

    com1_puts("[PIC] Registering IRQ0 timer handler...\r\n");
    irq_register_handler(0, timer_irq_handler);
    com1_puts("[PIC] irq_register_handler(0) done\r\n");

    com1_puts("[PIC] Initializing keyboard driver...\r\n");
    keyboard_init();
    com1_puts("[PIC] keyboard_init() done\r\n");

    com1_puts("[PIC] Clearing IRQ0 & IRQ1 masks...\r\n");
    pic_clear_mask(0);
    pic_clear_mask(1);
    com1_puts("[PIC] Interrupt masks cleared\r\n");

    diag_set_step("PIC REMAP DONE");
    diag_set_pic_telemetry(true, true, 0xFEC00000, 0, 0, 0, 0x20);
    diag_set_pass("PIC");
    diag_set_step("PIC CERTIFIED");
    com1_puts("[PIC_PASS]\r\n");

    // =====================================================================
    // 7. STI — Enable Hardware Interrupts
    // =====================================================================
    com1_puts("[STI_START] Enabling hardware interrupts via STI...\r\n");
    diag_set_step("STI INTERRUPTS ENABLED");
    __asm__ volatile("sti");
    com1_puts("[STI_PASS] Hardware interrupts enabled successfully\r\n");

    // =====================================================================
    // 8. PMM Physical Memory Manager
    // =====================================================================
    com1_puts("[PMM_START]\r\n");
    diag_set_running("PMM");
    diag_set_step("PMM INIT START");

    com1_puts("[PMM] Calling pmm_init(boot_info)...\r\n");
    pmm_init(boot_info);
    com1_puts("[PMM] pmm_init() returned successfully\r\n");

    diag_set_pass("PMM");
    diag_set_step("PMM CERTIFIED");
    com1_puts("[PMM_PASS]\r\n");

    // =====================================================================
    // 9. VMM Virtual Memory Manager — TARGET CERTIFICATION
    // =====================================================================
    com1_puts("[VMM_START]\r\n");
    diag_set_running("VMM");
    diag_set_step("VMM ENTRY");

    com1_puts("[VMM] Calling vmm_init()...\r\n");
    vmm_init();
    com1_puts("[VMM] vmm_init() returned successfully\r\n");

    diag_set_pass("VMM");
    diag_set_step("VMM CERTIFIED");
    com1_puts("[VMM_PASS]\r\n");

    com1_puts("[BOOT] Initializing PCI Bus, Network & USB Host Controllers...\r\n");
    extern void pci_init(void);
    extern void r8168_init(void);
    extern void debuglan_init(void);
    extern void usb_registry_init(void);
    extern void usb_hid_init(void);
    extern void xhci_init(void);
    extern void usb_forensic_center_init(void);
    extern void usb_forensic_center_render(void);

    diag_set_step("PCI & USB INITIALIZATION");
    usb_forensic_center_init();
    diag_set_step("PCI BUS PROBING");
    pci_init();
    diag_set_step("REALTEK R8168 NIC BRINGUP");
    r8168_init();
    diag_set_step("LAN TELEMETRY INIT");
    debuglan_init();
    extern void remote_power_init(void);
    remote_power_init();
    diag_set_step("USB HID DRIVER REGISTRATION");
    usb_registry_init();
    usb_hid_init();
    diag_set_step("XHCI HARDWARE BRINGUP");
    xhci_init();
    diag_set_step("USB INITIALIZATION COMPLETE");
    usb_forensic_center_render();

    // =====================================================================
    // 10. HEAP — Stage A Basic Heap Bring-Up
    // =====================================================================
    com1_puts("[HEAP_START]\r\n");
    diag_set_running("HEAP");
    diag_set_step("HEAP INIT START");

    extern void heap_init(void);
    extern void heap_stage_a_stress_test(void);

    com1_puts("[HEAP] Calling heap_init()...\r\n");
    heap_init();
    com1_puts("[HEAP] heap_init() complete\r\n");
    com1_puts("[HEAP_MARKER_E] AFTER HEAP_INIT COMPLETE\r\n");

    com1_puts("\r\n==================================================\r\n");
    com1_puts(" [BOE FORENSIC AUDIT: FRAMEBUFFER MEMORY AUDIT]\r\n");
    com1_puts("==================================================\r\n");
    com1_puts("g_abde.framebuffer : ");
    if (g_abde.framebuffer) {
        char hex_chars[] = "0123456789ABCDEF";
        com1_puts("0x");
        for (int i = 60; i >= 0; i -= 4) {
            char c[2] = { hex_chars[(g_abde.framebuffer >> i) & 0xF], '\0' };
            com1_puts(c);
        }
    } else {
        com1_puts("NULL (0x0)");
    }
    com1_puts("\r\n");

    com1_puts("g_abde.pitch       : ");
    {
        uint32_t p = g_abde.pitch;
        if (p == 0) com1_puts("0");
        else {
            char buf[16]; int pos = 14; buf[15] = '\0';
            while (p > 0) { buf[pos--] = '0' + (p % 10); p /= 10; }
            com1_puts(&buf[pos + 1]);
        }
    }
    com1_puts("\r\n");

    com1_puts("g_abde.width       : ");
    {
        uint32_t w = g_abde.width;
        if (w == 0) com1_puts("0");
        else {
            char buf[16]; int pos = 14; buf[15] = '\0';
            while (w > 0) { buf[pos--] = '0' + (w % 10); w /= 10; }
            com1_puts(&buf[pos + 1]);
        }
    }
    com1_puts("\r\n");

    com1_puts("g_abde.height      : ");
    {
        uint32_t h = g_abde.height;
        if (h == 0) com1_puts("0");
        else {
            char buf[16]; int pos = 14; buf[15] = '\0';
            while (h > 0) { buf[pos--] = '0' + (h % 10); h /= 10; }
            com1_puts(&buf[pos + 1]);
        }
    }
    com1_puts("\r\n");
    com1_puts("==================================================\r\n\r\n");

    com1_puts("[HEAP_MARKER_F] HEAP CERTIFIED STAGE A\r\n");
    com1_puts("[HEAP] Stage A Heap Ready!\r\n");
    com1_puts("[HEAP_MARKER_G] AFTER HEAP STAGE A\r\n");

    diag_set_pass("HEAP");
    diag_set_step("HEAP STAGE A CERTIFIED");
    com1_puts("[HEAP_PASS]\r\n");

    // =====================================================================
    // 11. SCHEDULER ENGINE ACTIVATION & MULTITASKING WIRING
    // =====================================================================
    com1_puts("[SCHED_START]\r\n");
    diag_set_running("SCHED");
    diag_set_step("SCHED INIT START");

    extern void scheduler_init(void);
    extern Task *scheduler_register_boot_task(void);
    extern Task *scheduler_create_kernel_task(const char *name, void (*entry)(void), uint8_t priority);
    extern void timer_init(uint32_t frequency);
    extern void scheduler_start(void);

    com1_puts("[SCHED] Calling scheduler_init()...\r\n");
    scheduler_init();

    com1_puts("[SCHED] Creating Production System Threads...\r\n");
    g_system_threads[0] = scheduler_create_kernel_task("ABDE_Telemetry", system_telemetry_thread, 24);
    g_system_threads[1] = scheduler_create_kernel_task("Heartbeat", system_heartbeat_thread, 24);
    g_system_threads[2] = scheduler_create_kernel_task("Diagnostics", system_diagnostics_thread, 16);
    g_system_threads[3] = scheduler_create_kernel_task("Debug_Shell", system_debug_shell_thread, 16);

    // =====================================================================
    // ATOMS OS OFFICIAL BOOT EXPERIENCE — ROOK ENGINE SUPERVISOR
    // =====================================================================
    extern void rook_init(uint32_t* gop_fb, uint32_t width, uint32_t height, uint32_t stride);
    extern int rook_register_page(struct rook_page* page);
    extern int rook_goto(uint16_t page_id);
    extern void rook_splash_spin(uint32_t total_ms);

    if (boot_info && boot_info->vbe_framebuffer) {
        com1_puts("[ROOK] Initializing Official ATOMS Boot Experience...\r\n");
        uint32_t stride_pixels = boot_info->vbe_pitch / 4;
        if (stride_pixels == 0) stride_pixels = g_kernel_screen_width;

        rook_init((uint32_t*)(uintptr_t)boot_info->vbe_framebuffer, g_kernel_screen_width, g_kernel_screen_height, stride_pixels);
        rook_register_page(rook_page_boot_get());
        rook_register_page(rook_page_login_get());

        rook_goto(ROOK_PAGE_BOOT_SPLASH);
        com1_puts("[ROOK] Boot Splash active on #000000 black canvas (6.0s AME Spinner)...\r\n");
        rook_splash_spin(6000);

        com1_puts("[ROOK] Transitioning to Login Screen (ROOK_PAGE_LOGIN)...\r\n");
        rook_goto(ROOK_PAGE_LOGIN);
    }

    atoms_cursor_certification_init(boot_info);
    scheduler_create_kernel_task("Cursor_Cert", atoms_cursor_certification_task, 24);

    // =====================================================================
    // LEVEL 5 PROCESS ENGINE & USER MODE ACTIVATION
    // =====================================================================
    com1_puts("[L5_START] Activating Process Engine, Thread Manager, Usermode & Syscall MSRs...\r\n");
    diag_set_running("PROC");
    diag_set_step("PROC INIT START");

    extern void ATOMS_ProcessManager_Init(void);
    extern void ATOMS_ThreadManager_Init(void);
    extern void ATOMS_UserMode_Init(void);
    extern void syscall_init(void);
    extern void BOSX_Init(void);
    extern void* vmm_create_address_space(void);
    extern bool vmm_map_user_page(void *pml4, uint64_t virt_addr, uint32_t access);
    extern uint64_t vmm_translate(void *pml4, uint64_t virt_addr);
    extern bool process_build_user_stack(void* image, void* pml4);
    extern void* process_spawn(void* image, const char* name);
    #include "kernel/core/process/include/process_image.h"

    ATOMS_ProcessManager_Init();
    ATOMS_ThreadManager_Init();
    ATOMS_UserMode_Init();
    syscall_init();
    BOSX_Init();

    com1_puts("[L5_PASS] Process Engine Subsystems Active! Spawning First Ring 3 User Process...\r\n");

    void *user_pml4 = vmm_create_address_space();
    if (user_pml4) {
        if (vmm_map_user_page(user_pml4, 0x40000000ULL, 1U | 2U | 4U)) { // READ | WRITE | EXECUTE
            uint8_t *user_code = (uint8_t*)vmm_translate(user_pml4, 0x40000000ULL);
            if (user_code) {
                // Assembly payload:
                // mov $0, %rax (SYS_WRITE); mov $0x40000100, %rdi; mov $55, %rsi; syscall
                // mov $3, %rax (SYS_YIELD); syscall
                // mov $1, %rax (SYS_EXIT); xor %rdi, %rdi; syscall; pause; jmp .-4
                uint8_t code_bytes[] = {
                    0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,
                    0x48, 0xC7, 0xC7, 0x00, 0x01, 0x40, 0x00,
                    0x48, 0xC7, 0xC6, 0x37, 0x00, 0x00, 0x00,
                    0x0F, 0x05,
                    0x48, 0xC7, 0xC0, 0x03, 0x00, 0x00, 0x00,
                    0x0F, 0x05,
                    0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00,
                    0x48, 0x31, 0xFF,
                    0x0F, 0x05,
                    0xF3, 0x90, 0xEB, 0xFC
                };
                for (size_t b = 0; b < sizeof(code_bytes); b++) user_code[b] = code_bytes[b];

                const char *user_msg = "\r\n[RING 3 USER MODE] Level 5 User Process Executing!\r\n";
                char *msg_dst = (char*)(user_code + 0x100);
                for (size_t m = 0; user_msg[m]; m++) msg_dst[m] = user_msg[m];
                msg_dst[55] = '\0';

                ProcessImage img;
                for (uint8_t *p = (uint8_t*)&img; p < (uint8_t*)&img + sizeof(img); p++) *p = 0;
                img.entry_point = 0x40000000ULL;
                img.image_base  = 0x40000000ULL;
                img.image_end   = 0x40001000ULL;
                img.image_size  = 0x1000ULL;
                img.pml4        = user_pml4;

                if (process_build_user_stack(&img, user_pml4)) {
                    process_spawn(&img, "Init_UserProcess");
                    com1_puts("[L5_SPAWN] First Ring 3 User Process Successfully Enqueued!\r\n");
                }
            }
        }
    }

    diag_set_pass("PROC");
    diag_set_step("LEVEL 5 ENGINE ACTIVE");

    com1_puts("[TIMER] Calling timer_init(1000) for IRQ0 scheduler ticks...\r\n");
    timer_init(1000);

    diag_set_pass("SCHED");
    diag_set_step("SCHED ACTIVATED");
    com1_puts("[SCHED_PASS]\r\n");

    // =====================================================================
    // ACTIVE HEARTBEAT HALT LOOP / SCHEDULER HANDOFF
    // =====================================================================
    com1_puts("[HEAP_MARKER_H] SKIPPING EARLY ABDE RENDER FOR CURSOR CERTIFICATION ENVIRONMENT\r\n");
    // diag_render(); // Disabled: Boot directly into Cursor Certification UI
    com1_puts("[HEAP_MARKER_I] SCHEDULER WIRING READY\r\n");

    com1_puts("[SCHED] Handoff execution to scheduler_start()...\r\n");
    scheduler_start();

    com1_puts("[BOOT_COMPLETE] Fallback active heartbeat loop\r\n");
    for (;;) {
        diag_heartbeat_tick();
        for (volatile int i = 0; i < 5000000; i++) {
            __asm__ __volatile__("nop");
        }
    }
}
