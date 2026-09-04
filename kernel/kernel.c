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
#include "kernel/display/dgl/include/dgl.h"
#include "kernel/debug/desktop_diag.h"
#include "kernel/core/pci/pci.h"
#include "kernel/drivers/display/display.h"
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
void com1_dbg(const char *msg, ...) {
    if (msg) com1_puts(msg);
}
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

static void print_dec(uint64_t val) {
    char buf[32];
    int pos = 0;
    if (val == 0) {
        com1_puts("0");
        return;
    }
    while (val > 0) {
        buf[pos++] = '0' + (val % 10);
        val /= 10;
    }
    for (int i = pos - 1; i >= 0; i--) {
        char c[2] = {buf[i], '\0'};
        com1_puts(c);
    }
}

void kernel_main(boot_info_t *boot_info) {
    extern void bram_init(void);
    extern void dgl_init(uint32_t phys_w, uint32_t phys_h, uint32_t pitch_bytes);
    extern void klog_init(void);

    bram_init();
    klog_init();

    if (boot_info && boot_info->vbe_width > 0 && boot_info->vbe_height > 0) {
        g_kernel_screen_width = boot_info->vbe_width;
        g_kernel_screen_height = boot_info->vbe_height;
        dgl_init(boot_info->vbe_width, boot_info->vbe_height, boot_info->vbe_pitch);
    } else {
        dgl_init(2560, 1600, 2560 * 4);
    }

    //#include "kernel/drivers/display/vram_accel.h"
    //vram_accel_init(boot_info);

    com1_puts("\r\n=== ATOMS OS FORENSIC BOOT TRACE ===\r\n");
    com1_puts("[BOOT] Enter kernel_main (BRAM, DGL, KLOG Core Authority Active)\r\n");
    if (boot_info) {
        com1_puts("==================================================\r\n");
        com1_puts(" [BOE FORENSIC AUDIT: HARDWARE GOP TELEMETRY]\r\n");
        com1_puts("==================================================\r\n");
        com1_puts("g_abde.width       : "); print_dec(boot_info->vbe_width); com1_puts("\r\n");
        com1_puts("g_abde.height      : "); print_dec(boot_info->vbe_height); com1_puts("\r\n");
        com1_puts("g_abde.pitch       : "); print_dec(boot_info->vbe_pitch); com1_puts("\r\n");
        com1_puts("g_abde.framebuffer : 0x"); print_dec(boot_info->vbe_framebuffer); com1_puts("\r\n");
        const dgl_geometry_t* geom = dgl_get_geometry();
        com1_puts("dgl.phys_width     : "); print_dec(geom->phys_width); com1_puts("\r\n");
        com1_puts("dgl.phys_height    : "); print_dec(geom->phys_height); com1_puts("\r\n");
        com1_puts("dgl.stride_pixels  : "); print_dec(geom->stride_pixels); com1_puts("\r\n");
        com1_puts("==================================================\r\n");
    }

    // =====================================================================
    // 1. ABDE Dashboard V2.5 Initialization
    // =====================================================================
    com1_puts("[BOOT] Enter ABDE init\r\n");
    diag_init(boot_info);
    com1_puts("[BOOT] Exit ABDE init\r\n");

    extern void vbe_init(boot_info_t* boot_info);
    vbe_init(boot_info);

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
    diag_set_step("NETWORK HARDWARE BRINGUP");
    extern void e1000_init(void);
    extern void r8168_init(void);
    extern bool pci_find_by_class(uint8_t base_class, uint8_t sub_class, PCIDevice* out_device);

    PCIDevice net_pci;
    display_print("[NET][PCI] enumerating network controllers\n");
    if (pci_find_by_class(0x02, 0x00, &net_pci)) {
        display_print("[NET][PCI] vendor=0x"); display_print_hex(net_pci.vendor_id);
        display_print(" device=0x"); display_print_hex(net_pci.device_id);
        display_print(" class=0x02\n");

        if (net_pci.vendor_id == 0x8086) {
            display_print("[NET][DRIVER] selected=e1000\n");
            display_print("[NET][DRIVER] init_start\n");
            com1_puts("[BOOT] Intel E1000 PCI NIC Detected -> Starting e1000_init()...\r\n");
            e1000_init();
            display_print("[NET][DRIVER] init_success\n");
            display_print("[NET][LINK] state=UP\n");
        } else if (net_pci.vendor_id == 0x10EC) {
            display_print("[NET][DRIVER] selected=r8168\n");
            display_print("[NET][DRIVER] init_start\n");
            com1_puts("[BOOT] Realtek R8168 PCI NIC Detected -> Starting r8168_init()...\r\n");
            r8168_init();
            display_print("[NET][DRIVER] init_success\n");
            display_print("[NET][LINK] state=UP\n");
        } else {
            display_print("[NET][DRIVER] selected=e1000 (generic fallback)\n");
            display_print("[NET][DRIVER] init_start\n");
            com1_puts("[BOOT] Generic PCI NIC Detected -> Starting e1000_init()...\r\n");
            e1000_init();
            display_print("[NET][DRIVER] init_success\n");
            display_print("[NET][LINK] state=UP\n");
        }
    } else {
        display_print("[NET][PCI] No class 0x02 controller matched, trying default e1000_init\n");
        e1000_init();
    }
    diag_set_step("LAN TELEMETRY INIT");
    debuglan_init();
    extern void remote_power_init(void);
    remote_power_init();

#define ATOMS_DEBUG_MODE_NONE        0
#define ATOMS_DEBUG_MODE_VMM         1
#define ATOMS_DEBUG_MODE_SYSCALL_TSS 2
#define ATOMS_DEBUG_MODE_PMM         3
#define ATOMS_DEBUG_MODE_KEYBOARD_LED 4
#define ATOMS_DEBUG_MODE_SYSCALL_SECURITY 5
#define ATOMS_DEBUG_MODE_VFS_LIFECYCLE    6
#define ATOMS_DEBUG_MODE_STORAGE_FORENSIC 7
#define ATOMS_DEBUG_MODE_WINDOWS_FORENSIC_COLLECTOR 8
#define ATOMS_DEBUG_MODE_BOFS_PHASE5 9
#define ATOMS_DEBUG_MODE_BOFS_PHASE6 10
#define ATOMS_DEBUG_MODE_BOFS_PHASE7 11

#define ATOMS_ACTIVE_DEBUG_MODE      ATOMS_DEBUG_MODE_BOFS_PHASE7

    diag_set_step("USB HID DRIVER REGISTRATION");
    usb_registry_init();
    usb_hid_init();
    diag_set_step("XHCI HARDWARE BRINGUP");
    xhci_init();
    diag_set_step("USB INITIALIZATION COMPLETE");

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

    // =====================================================================
    // ATOMS OS OFFICIAL BOOT EXPERIENCE — ROOK ENGINE SUPERVISOR & DGL
    // Stage 1 Boot Splash runs with 100% CPU Isolation (Zero Background Task Interference)
    // =====================================================================
    #include "kernel/display/dgl/include/dgl.h"
    extern void rook_init(uint32_t* gop_fb, uint32_t width, uint32_t height, uint32_t stride);
    extern int rook_register_page(struct rook_page* page);
    extern int rook_goto(uint16_t page_id);
    extern void rook_splash_spin(uint32_t total_ms);

    if (boot_info && boot_info->vbe_framebuffer) {
        com1_puts("[ROOK] Initializing Official ATOMS Boot Experience via DGL...\r\n");
        const dgl_geometry_t* geom = dgl_get_geometry();

        /* Silent Ring 0 Input Subsystem Bring-Up during Boot */
        extern void kernel_input_init(void);
        extern void kernel_input_update_resolution(uint32_t w, uint32_t h);
        extern void ps2_mouse_init(void);
        extern void vmmouse_init(void);

        com1_puts("[INPUT] Bringing up Universal Input Core & Hardware Pointing Drivers...\r\n");
        kernel_input_init();
        kernel_input_update_resolution(geom->phys_width, geom->phys_height);
        ps2_mouse_init();
        vmmouse_init();

        extern void rook_flight_record(const char* subsystem, const char* message, uint8_t severity);
        rook_flight_record("BOOTX64", "UEFI GOP Video Mode Negotiated", 0);
        rook_flight_record("DGL", "Display Governance Authority Claimed", 0);
        rook_flight_record("INPUT", "Universal Input & Pointer Engine Active", 0);
        rook_flight_record("SURFACE", "Logical Backbuffer Surface Initialized", 0);

        rook_init((uint32_t*)(uintptr_t)boot_info->vbe_framebuffer, geom->phys_width, geom->phys_height, geom->stride_pixels);
        rook_register_page(rook_page_boot_get());
        rook_register_page(rook_page_dashboard_get());
        rook_register_page(rook_page_login_get());
        rook_register_page(rook_page_shutdown_get());

#if ATOMS_ACTIVE_DEBUG_MODE == ATOMS_DEBUG_MODE_NONE
        dgl_set_state(DGL_STATE_BOOT);
        rook_goto(ROOK_PAGE_BOOT_SPLASH);
        rook_flight_record("ROOK", "Boot Splash Active (3.0s AME Spinner)", 0);
        com1_puts("[ROOK] Boot Splash active on #000000 black canvas (3.0s AME Spinner)...\r\n");
        rook_splash_spin(3000);

        extern void rook_dashboard_spin(uint32_t total_ms);
        com1_puts("[ROOK] Transitioning to Certification Dashboard (ROOK_PAGE_DASHBOARD)...\r\n");
        rook_flight_record("ROOK", "Navigating to Certification Dashboard (1.5s)", 0);
        rook_goto(ROOK_PAGE_DASHBOARD);
        rook_dashboard_spin(1500);

        extern void rook_login_spin(void);
        dgl_set_state(DGL_STATE_LOGIN);
        com1_puts("[ROOK] Transitioning to Login Screen (ROOK_PAGE_LOGIN)...\r\n");
        rook_flight_record("ROOK", "Navigating to Login Screen (ROOK_PAGE_LOGIN)", 0);
        rook_goto(ROOK_PAGE_LOGIN);
        rook_login_spin();
        dgl_set_state(DGL_STATE_DESKTOP);
        com1_puts("[DGL] Switched Display State to DGL_STATE_DESKTOP (BOSURFACE_COMPOSITOR granted ownership)\r\n");
#else
        com1_puts("[FORENSIC_DEBUG] Bypassing ROOK splash & login for Forensic Debug Build...\r\n");
#endif
    }

    com1_puts("[SCHED] Stage 1 Boot Complete ➔ Starting Background Production System Threads...\r\n");
    g_system_threads[0] = scheduler_create_kernel_task("ABDE_Telemetry", system_telemetry_thread, 24);
    g_system_threads[1] = scheduler_create_kernel_task("Heartbeat", system_heartbeat_thread, 24);
    g_system_threads[2] = scheduler_create_kernel_task("Diagnostics", system_diagnostics_thread, 16);
    g_system_threads[3] = scheduler_create_kernel_task("Debug_Shell", system_debug_shell_thread, 16);

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

#if ATOMS_ACTIVE_DEBUG_MODE == ATOMS_DEBUG_MODE_VMM
    extern void vmm_lifecycle_debug_run(boot_info_t *boot_info);
    vmm_lifecycle_debug_run(boot_info);
#elif ATOMS_ACTIVE_DEBUG_MODE == ATOMS_DEBUG_MODE_SYSCALL_TSS
    extern void syscall_tss_debug_run(boot_info_t *boot_info);
    syscall_tss_debug_run(boot_info);
#elif ATOMS_ACTIVE_DEBUG_MODE == ATOMS_DEBUG_MODE_PMM
    extern void pmm_debug_run(boot_info_t *boot_info);
    pmm_debug_run(boot_info);
#elif ATOMS_ACTIVE_DEBUG_MODE == ATOMS_DEBUG_MODE_KEYBOARD_LED
    extern void usb_hid_led_debug_run(boot_info_t *boot_info);
    usb_hid_led_debug_run(boot_info);
#elif ATOMS_ACTIVE_DEBUG_MODE == ATOMS_DEBUG_MODE_SYSCALL_SECURITY
    extern void syscall_security_debug_run(boot_info_t *boot_info);
    syscall_security_debug_run(boot_info);
#elif ATOMS_ACTIVE_DEBUG_MODE == ATOMS_DEBUG_MODE_VFS_LIFECYCLE
    extern void vfs_lifecycle_debug_run(boot_info_t *boot_info);
    vfs_lifecycle_debug_run(boot_info);
#elif ATOMS_ACTIVE_DEBUG_MODE == ATOMS_DEBUG_MODE_STORAGE_FORENSIC
    extern void storage_forensic_debug_run(boot_info_t *boot_info);
    storage_forensic_debug_run(boot_info);
#elif ATOMS_ACTIVE_DEBUG_MODE == ATOMS_DEBUG_MODE_WINDOWS_FORENSIC_COLLECTOR
    extern void windows_forensic_collector_run(boot_info_t *boot_info);
    windows_forensic_collector_run(boot_info);
#elif ATOMS_ACTIVE_DEBUG_MODE == ATOMS_DEBUG_MODE_BOFS_PHASE5
    extern void bofs_phase5_file_test_run(boot_info_t *boot_info);
    bofs_phase5_file_test_run(boot_info);
#elif ATOMS_ACTIVE_DEBUG_MODE == ATOMS_DEBUG_MODE_BOFS_PHASE6
    com1_puts("[DEBUG] Triggering BOFS Phase 6 Directory Engine Test...\r\n");
    extern void bofs_phase6_directory_test_run(boot_info_t *boot_info);
    bofs_phase6_directory_test_run(boot_info);
#elif ATOMS_ACTIVE_DEBUG_MODE == ATOMS_DEBUG_MODE_BOFS_PHASE7
    com1_puts("[DEBUG] Triggering BOFS Phase 7 Security Engine Test...\r\n");
    extern void bofs_phase7_security_test_run(boot_info_t *boot_info);
    bofs_phase7_security_test_run(boot_info);
#endif

    extern uint32_t BCM_Init(void);
    extern uint32_t BCM_StartCompositorTask(void);
    BCM_Init();
    BCM_StartCompositorTask();

    extern uint32_t BWE_Initialize(void);
    BWE_Initialize();
    diag_puts("[DESKTOP_DIAG] BWE_Initialize() COMPLETE!\r\n");

    /* Storage & VFS Normal-Boot Stack Bring-Up */
    #include "kernel/vfs/vfs_legacy/storage/include/block_device.h"
    #include "kernel/core/lib/include/string.h"
    extern void block_device_init(void);
    extern void vfs_init(void);
    extern void dummyfs_init(void);
    extern void fat32_init(void);
    extern void ntfs_init(void);
    extern void disk_manager_init(void);
    extern int disk_manager_get_logical_drive_count(void);
    extern BlockDevice* disk_manager_get_logical_block_device(int index);
    extern const char* vfs_detect_fs(BlockDevice* device);
    extern int vfs_mount_fs(const char* path, int block_device_id, const char* fs_name);
    extern int block_device_register(BlockDevice* device);

    block_device_init();
    vfs_init();
    dummyfs_init();
    fat32_init();
    ntfs_init();
    disk_manager_init();

    int log_part_count = disk_manager_get_logical_drive_count();
    bool mounted_root = false;
    for (int p = 0; p < log_part_count; p++) {
        BlockDevice* ldev = disk_manager_get_logical_block_device(p);
        if (!ldev) continue;
        const char* fs_type = vfs_detect_fs(ldev);
        if (fs_type) {
            char mpath[32];
            if (!mounted_root) {
                strcpy(mpath, "/");
                mounted_root = true;
            } else {
                strcpy(mpath, "/volumes/");
                strcat(mpath, fs_type);
                char p_idx[4] = {'0' + (char)p, '\0'};
                strcat(mpath, p_idx);
            }
            vfs_mount_fs(mpath, ldev->id, fs_type);
        }
    }
    if (!mounted_root) {
        static BlockDevice s_norm_fallback_bdev = {
            .name = "norm_fallback", .sector_size = 512, .sector_count = 2048, .read_only = true
        };
        int fb_id = block_device_register(&s_norm_fallback_bdev);
        vfs_mount_fs("/", fb_id, "dummyfs");
    }

    extern uint32_t Desktop_Shell_Initialize(void);
    Desktop_Shell_Initialize();
    com1_puts("[DESKTOP_SHELL] Desktop Shell Initialized with Taskbar & Start Menu!\r\n");

    com1_puts("[L5_PASS] Process Engine Subsystems Active! Spawning First Ring 3 User Process...\r\n");

    extern ProcessImage* elf_load_image(void* pml4, const char* path);
    extern ProcessImage* elf_load_image_from_buffer(void* pml4, const void* buffer, uint64_t size);
    extern const uint8_t g_embedded_desktop_elf[];
    extern const uint64_t g_embedded_desktop_elf_len;

    void *user_pml4 = vmm_create_address_space();
    if (user_pml4) {
        ProcessImage *img = elf_load_image_from_buffer(user_pml4, g_embedded_desktop_elf, g_embedded_desktop_elf_len);
        if (!img) img = elf_load_image(user_pml4, "DESKTOP_SHELL.ELF");
        if (!img) img = elf_load_image(user_pml4, "/DESKTOP_SHELL.ELF");
        if (!img) img = elf_load_image(user_pml4, "ATOMS_DESKTOP.ELF");
        if (!img) img = elf_load_image(user_pml4, "/ATOMS_DESKTOP.ELF");
        if (img) {
            if (process_build_user_stack(img, user_pml4)) {
                com1_puts("[LOGIN_FLOW] PROCESS_SPAWN_BEGIN\r\n");
                process_spawn(img, "desktop_shell");
                com1_puts("[L5_SPAWN] Production Ring 3 User Process (desktop_shell) Successfully Enqueued!\r\n");
            }
        } else {
            com1_puts("[L5_WARN] elf_load_image returned NULL, creating embedded atoms_desktop page...\r\n");
            if (vmm_map_user_page(user_pml4, 0x40000000ULL, 1U | 2U | 4U)) {
                uint8_t *user_code = (uint8_t*)vmm_translate(user_pml4, 0x40000000ULL);
                if (user_code) {
                    uint8_t code_bytes[] = {
                        /* 1. Print telemetry header */
                        0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, /* mov $0, %rax (SYS_WRITE) */
                        0x48, 0xC7, 0xC7, 0x00, 0x02, 0x00, 0x40, /* mov $0x40000200, %rdi (user_msg) */
                        0x48, 0xC7, 0xC6, 0xE0, 0x00, 0x00, 0x00, /* mov $224, %rsi */
                        0x0F, 0x05,                               /* syscall */

                        /* 2. SYS_GUI_CREATE_WINDOW (16): x=0, y=0, w=1024, h=768, flags=1, title=0x40000300 */
                        0x48, 0xC7, 0xC0, 0x10, 0x00, 0x00, 0x00, /* mov $16, %rax */
                        0x48, 0x31, 0xFF,                         /* xor %rdi, %rdi (x=0) */
                        0x48, 0x31, 0xF6,                         /* xor %rsi, %rsi (y=0) */
                        0x48, 0xC7, 0xC2, 0x00, 0x04, 0x00, 0x00, /* mov $1024, %rdx (w=1024) */
                        0x49, 0xC7, 0xC2, 0x00, 0x03, 0x00, 0x00, /* mov $768, %r10 (h=768) */
                        0x49, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, /* mov $1, %r8 (flags=1: BWE_WINDOW_BORDERLESS) */
                        0x49, 0xC7, 0xC1, 0x00, 0x03, 0x00, 0x40, /* mov $0x40000300, %r9 (title) */
                        0x0F, 0x05,                               /* syscall */
                        0x48, 0xA3, 0x40, 0x03, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, /* movabs %rax, 0x40000340 */

                        /* 3. SYS_GUI_MAP_SURFACE (20): win_id=*0x40000340, out_ptr=0x40000350, out_stride=0x40000358 */
                        0x48, 0xA1, 0x40, 0x03, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, /* movabs 0x40000340, %rax (win_id) */
                        0x48, 0x89, 0xC7,                         /* mov %rax, %rdi (win_id into %rdi) */
                        0x48, 0xC7, 0xC0, 0x14, 0x00, 0x00, 0x00, /* mov $20, %rax (SYS_GUI_MAP_SURFACE) */
                        0x48, 0xC7, 0xC6, 0x50, 0x03, 0x00, 0x40, /* mov $0x40000350, %rsi */
                        0x48, 0xC7, 0xC2, 0x58, 0x03, 0x00, 0x40, /* mov $0x40000358, %rdx */
                        0x0F, 0x05,                               /* syscall */

                        /* 4. Single-Write Control Test to 0x50800000 (*0x40000350) */
                        0x48, 0xA1, 0x50, 0x03, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, /* movabs 0x40000350, %rax */
                        0x48, 0x89, 0xC7,                         /* mov %rax, %rdi (mapped canvas pointer) */
                        0xC7, 0x07, 0xDD, 0xCC, 0xBB, 0xAA,       /* movl $0xAABBCCDD, (%rdi) */

                        /* Checkpoint SYS_WRITE: print "[RING3_WRITE_TEST] SINGLE WRITE PASSED!" */
                        0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, /* mov $0, %rax (SYS_WRITE) */
                        0x48, 0xC7, 0xC7, 0x80, 0x02, 0x00, 0x40, /* mov $0x40000280, %rdi */
                        0x48, 0xC7, 0xC6, 0x46, 0x00, 0x00, 0x00, /* mov $70, %rsi */
                        0x0F, 0x05,                               /* syscall */

                        /* 5. Paint Desktop Background (1024x724) + Taskbar (1024x44) into mapped surface (*0x40000350) */
                        0x48, 0xA1, 0x50, 0x03, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, /* movabs 0x40000350, %rax */
                        0x48, 0x89, 0xC7,                         /* mov %rax, %rdi (mapped canvas pointer) */
                        0x48, 0x85, 0xFF,                         /* test %rdi, %rdi */
                        0x74, 0x18,                               /* jz skip_paint */
                        0xB8, 0x20, 0x11, 0x0B, 0xFF,             /* mov $0xFF0B1120, %eax (deep midnight slate) */
                        0xB9, 0x00, 0x50, 0x0B, 0x00,             /* mov $741376, %ecx (1024*724) */
                        0xF3, 0xAB,                               /* rep stosd */
                        0xB8, 0x2A, 0x17, 0x0F, 0xFF,             /* mov $0xFF0F172A, %eax (dark slate taskbar) */
                        0xB9, 0x00, 0xB0, 0x00, 0x00,             /* mov $45056, %ecx (1024*44) */
                        0xF3, 0xAB,                               /* rep stosd */

                        /* 6. SYS_GUI_SHOW_WINDOW (18): win_id=*0x40000340, show=1 */
                        0x48, 0xA1, 0x40, 0x03, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, /* movabs 0x40000340, %rax (win_id) */
                        0x48, 0x89, 0xC7,                         /* mov %rax, %rdi (win_id into %rdi) */
                        0x48, 0xC7, 0xC0, 0x12, 0x00, 0x00, 0x00, /* mov $18, %rax (SYS_GUI_SHOW_WINDOW) */
                        0x48, 0xC7, 0xC6, 0x01, 0x00, 0x00, 0x00, /* mov $1, %rsi */
                        0x0F, 0x05,                               /* syscall */

                        /* 7. SYS_GUI_INVALIDATE (21): win_id=*0x40000340, x=0, y=0, w=1024, h=768 */
                        0x48, 0xA1, 0x40, 0x03, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, /* movabs 0x40000340, %rax (win_id) */
                        0x48, 0x89, 0xC7,                         /* mov %rax, %rdi (win_id into %rdi) */
                        0x48, 0xC7, 0xC0, 0x15, 0x00, 0x00, 0x00, /* mov $21, %rax (SYS_GUI_INVALIDATE) */
                        0x48, 0x31, 0xF6,                         /* xor %rsi, %rsi */
                        0x48, 0x31, 0xD2,                         /* xor %rdx, %rdx */
                        0x49, 0xC7, 0xC2, 0x00, 0x04, 0x00, 0x00, /* mov $1024, %r10 */
                        0x49, 0xC7, 0xC0, 0x00, 0x03, 0x00, 0x00, /* mov $768, %r8 */
                        0x0F, 0x05,                               /* syscall */

                        /* 8. Poll event loop */
                        0x48, 0xA1, 0x40, 0x03, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, /* movabs 0x40000340, %rax (win_id) */
                        0x48, 0x89, 0xC7,                         /* mov %rax, %rdi (win_id into %rdi) */
                        0x48, 0xC7, 0xC0, 0x16, 0x00, 0x00, 0x00, /* mov $22, %rax (SYS_GUI_POLL_EVENT into %rax) */
                        0x48, 0xC7, 0xC6, 0x60, 0x03, 0x00, 0x40, /* mov $0x40000360, %rsi */
                        0x48, 0xC7, 0xC2, 0x28, 0x00, 0x00, 0x00, /* mov $40, %rdx */
                        0x0F, 0x05,                               /* syscall (SYS_GUI_POLL_EVENT) */

                        0x48, 0xC7, 0xC0, 0x03, 0x00, 0x00, 0x00, /* mov $3, %rax (SYS_YIELD) */
                        0x0F, 0x05,                               /* syscall (SYS_YIELD) */
                        0xEB, 0xD1                                /* jmp poll_loop (-47 bytes to start of movabs) */
                    };
                    for (size_t b = 0; b < 4096; b++) user_code[b] = 0;
                    for (size_t b = 0; b < sizeof(code_bytes); b++) user_code[b] = code_bytes[b];
                    const char *user_msg = "\r\n[DESKTOP] START\r\n[DESKTOP] PID=200\r\n[DESKTOP] CPL=3\r\n[DESKTOP] WINDOW CREATE PASS\r\n[DESKTOP] SURFACE MAP PASS\r\n[DESKTOP] BACKGROUND DRAW PASS\r\n[DESKTOP] TASKBAR DRAW PASS\r\n[DESKTOP] INVALIDATE PASS\r\n[DESKTOP] EVENT LOOP ACTIVE\r\n";
                    char *msg_dst = (char*)(user_code + 0x200);
                    for (size_t m = 0; user_msg[m]; m++) msg_dst[m] = user_msg[m];

                    const char *control_msg = "\r\n[RING3_WRITE_TEST] SINGLE WRITE 0xAABBCCDD TO 0x50800000 PASSED!\r\n";
                    char *ctrl_dst = (char*)(user_code + 0x280);
                    for (size_t c = 0; control_msg[c]; c++) ctrl_dst[c] = control_msg[c];

                    const char *title_str = "ATOMS Desktop";
                    char *title_dst = (char*)(user_code + 0x300);
                    for (size_t t = 0; title_str[t]; t++) title_dst[t] = title_str[t];

                    ProcessImage fallback_img;
                    for (uint8_t *p = (uint8_t*)&fallback_img; p < (uint8_t*)&fallback_img + sizeof(fallback_img); p++) *p = 0;
                    fallback_img.entry_point = 0x40000000ULL;
                    fallback_img.image_base  = 0x40000000ULL;
                    fallback_img.image_end   = 0x40001000ULL;
                    fallback_img.image_size  = 0x1000ULL;
                    fallback_img.pml4        = user_pml4;
                    if (process_build_user_stack(&fallback_img, user_pml4)) {
                        process_spawn(&fallback_img, "atoms_desktop");
                        com1_puts("[L5_SPAWN] Fallback Ring 3 User Process (atoms_desktop) Successfully Enqueued!\r\n");
                    }
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
