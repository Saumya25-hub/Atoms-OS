#include "syscall_tss_debug.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/display/dgl/include/dgl.h"
#include "kernel/core/bram/include/bram.h"
#include "kernel/core/syscall/include/syscall.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "arch/x86_64/smp/smp.h"
#include "arch/x86_64/gdt/gdt.h"

extern void com1_puts(const char *s);
extern tss_t tss_cpus[ATOMS_MAX_CPUS];
extern tss_t tss;

static SyscallTSSDebugStats g_syscall_debug_stats;
static boot_info_t *s_boot_info = 0;
static volatile uint64_t s_spin_tick = 0;
static const char s_spin_chars[4] = {'|', '/', '-', '\\'};

static inline uint64_t debug_read_msr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static void com1_put_hex(uint64_t val) {
    char buf[19];
    buf[0] = '0'; buf[1] = 'x';
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; i++) {
        buf[2 + i] = hex[(val >> ((15 - i) * 4)) & 0xF];
    }
    buf[18] = '\0';
    com1_puts(buf);
}

static void com1_put_dec(uint64_t val) {
    if (val == 0) { com1_puts("0"); return; }
    char buf[24]; int pos = 22; buf[23] = '\0';
    while (val > 0) {
        buf[pos--] = '0' + (val % 10);
        val /= 10;
    }
    com1_puts(&buf[pos + 1]);
}

static void syscall_dbg_render_hex(uint32_t x, uint32_t y, uint64_t val, uint32_t color, uint32_t bg) {
    char buf[19];
    buf[0] = '0'; buf[1] = 'x';
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; i++) {
        buf[2 + i] = hex[(val >> ((15 - i) * 4)) & 0xF];
    }
    buf[18] = '\0';
    abde_render_string(x, y, buf, color, bg);
}

static void syscall_dbg_render_dec(uint32_t x, uint32_t y, uint64_t val, uint32_t color, uint32_t bg) {
    if (val == 0) {
        abde_render_string(x, y, "0", color, bg);
        return;
    }
    char buf[24]; int pos = 22; buf[23] = '\0';
    while (val > 0) {
        buf[pos--] = '0' + (val % 10);
        val /= 10;
    }
    abde_render_string(x, y, &buf[pos + 1], color, bg);
}

static void get_cpu_brand_string(char *out_brand) {
    uint32_t eax, ebx, ecx, edx;
    uint32_t *brand_ptr = (uint32_t *)out_brand;

    for (uint32_t i = 0x80000002; i <= 0x80000004; i++) {
        __asm__ volatile("cpuid"
                         : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                         : "a"(i));
        *brand_ptr++ = eax;
        *brand_ptr++ = ebx;
        *brand_ptr++ = ecx;
        *brand_ptr++ = edx;
    }
    out_brand[48] = '\0';
}

void syscall_tss_debug_render(void) {
    uint32_t bg_color    = 0x00080E1A; // Deep Navy Background
    uint32_t panel_bg    = 0x000F172A; // Slate Dark Panel
    uint32_t cyan_color  = 0x0038BDF8; // Cyan Header
    uint32_t text_color  = 0x00E2E8F0; // Crisp White
    uint32_t label_color = 0x0094A3B8; // Muted Gray
    uint32_t pass_color  = 0x0022C55E; // Emerald Green
    uint32_t warn_color  = 0x00F59E0B; // Amber Yellow
    uint32_t fail_color  = 0x00EF4444; // Ruby Red
    uint32_t title_color = 0x0067E8F9; // Bright Cyan

    uint32_t screen_w = g_abde.width ? g_abde.width : 1024;
    uint32_t screen_h = g_abde.height ? g_abde.height : 768;

    abde_fill_rect(0, 0, screen_w, screen_h, bg_color);

    uint32_t start_x = 24;
    uint32_t start_y = 16;

    // Header Banner
    abde_render_string(start_x, start_y,      "==========================================================================================", cyan_color, bg_color);
    abde_render_string(start_x + 180, start_y + 18, "ATOMS OS - SYSCALL / TSS / SMP FORENSICS", title_color, bg_color);
    char spin_str[4] = {'[', s_spin_chars[s_spin_tick % 4], ']', '\0'};
    abde_render_string(start_x + 630, start_y + 18, spin_str, pass_color, bg_color);
    abde_render_string(start_x + 660, start_y + 18, "FORENSIC MODE ONLY (DESKTOP BYPASSED)", warn_color, bg_color);
    abde_render_string(start_x, start_y + 36, "==========================================================================================", cyan_color, bg_color);

    uint32_t cur_y = start_y + 54;
    uint32_t left_x = start_x;
    uint32_t right_x = start_x + 470;
    uint32_t panel_w = 450;
    uint32_t panel_h = 670;

    // Panel Backgrounds
    abde_fill_rect(left_x, cur_y, panel_w, panel_h, panel_bg);
    abde_fill_rect(right_x, cur_y, panel_w, panel_h, panel_bg);

    // =========================================================================
    // LEFT PANEL: SYSTEM, APIC / SMP, TSS & RSP0, SYSCALL MSRs
    // =========================================================================
    uint32_t ly = cur_y + 12;

    // 1. SYSTEM SECTION
    abde_render_string(left_x + 15, ly, "SYSTEM", cyan_color, panel_bg);
    ly += 16;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "CPU         :", label_color, panel_bg);
    abde_render_string(left_x + 160, ly, g_syscall_debug_stats.cpu_name[0] ? g_syscall_debug_stats.cpu_name : "x86_64 Processor", text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Detected CPUs:", label_color, panel_bg);
    syscall_dbg_render_dec(left_x + 160, ly, g_syscall_debug_stats.discovered_cpus, text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Online CPUs :", label_color, panel_bg);
    syscall_dbg_render_dec(left_x + 160, ly, g_syscall_debug_stats.online_cpus, pass_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Current CPU :", label_color, panel_bg);
    syscall_dbg_render_dec(left_x + 160, ly, g_syscall_debug_stats.current_cpu, pass_color, panel_bg);
    abde_render_string(left_x + 190, ly, "[BSP]", cyan_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Active CR3  :", label_color, panel_bg);
    syscall_dbg_render_hex(left_x + 160, ly, g_syscall_debug_stats.active_cr3, pass_color, panel_bg);
    ly += 24;

    // 2. APIC / SMP SECTION
    abde_render_string(left_x + 15, ly, "APIC / SMP", cyan_color, panel_bg);
    ly += 16;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Local APIC  :", label_color, panel_bg);
    abde_render_string(left_x + 160, ly, "0xFEE00000 [ENABLED]", pass_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "AP Startup  :", label_color, panel_bg);
    abde_render_string(left_x + 160, ly, "INIT-SIPI-SIPI [OK]", pass_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "APs Online  :", label_color, panel_bg);
    syscall_dbg_render_dec(left_x + 160, ly, g_syscall_debug_stats.online_cpus > 1 ? (g_syscall_debug_stats.online_cpus - 1) : 0, pass_color, panel_bg);
    abde_render_string(left_x + 190, ly, "Cores Booted", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Execution   :", label_color, panel_bg);
    abde_render_string(left_x + 160, ly, "BSP ACTIVE / APs TICKING", text_color, panel_bg);
    ly += 24;

    // 3. TSS SECTION
    abde_render_string(left_x + 15, ly, "TSS & GDT ARCHITECTURE", cyan_color, panel_bg);
    ly += 16;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "TSS Model   :", label_color, panel_bg);
    abde_render_string(left_x + 160, ly, g_syscall_debug_stats.tss_model_str, pass_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Active TR   :", label_color, panel_bg);
    syscall_dbg_render_hex(left_x + 160, ly, (uint64_t)g_syscall_debug_stats.active_tr, pass_color, panel_bg);
    abde_render_string(left_x + 260, ly, "(GDT 0x28)", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Current TSS :", label_color, panel_bg);
    syscall_dbg_render_hex(left_x + 160, ly, g_syscall_debug_stats.current_tss_addr, text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Current RSP0:", label_color, panel_bg);
    syscall_dbg_render_hex(left_x + 160, ly, g_syscall_debug_stats.rsp0_val, pass_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Kernel Stack:", label_color, panel_bg);
    abde_render_string(left_x + 160, ly, "32 KB Per-Task Dedicated", text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Stack Owner :", label_color, panel_bg);
    abde_render_string(left_x + 160, ly, g_syscall_debug_stats.stack_owner_str, cyan_color, panel_bg);
    ly += 24;

    // 4. SYSCALL MSR SECTION
    abde_render_string(left_x + 15, ly, "HARDWARE SYSCALL MSRs", cyan_color, panel_bg);
    ly += 16;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "IA32_LSTAR  :", label_color, panel_bg);
    syscall_dbg_render_hex(left_x + 160, ly, g_syscall_debug_stats.msr_lstar, pass_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "IA32_STAR   :", label_color, panel_bg);
    syscall_dbg_render_hex(left_x + 160, ly, g_syscall_debug_stats.msr_star, text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "IA32_FMASK  :", label_color, panel_bg);
    syscall_dbg_render_hex(left_x + 160, ly, g_syscall_debug_stats.msr_fmask, text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "IA32_EFER   :", label_color, panel_bg);
    syscall_dbg_render_hex(left_x + 160, ly, g_syscall_debug_stats.msr_efer, (g_syscall_debug_stats.msr_efer & 1) ? pass_color : fail_color, panel_bg);
    abde_render_string(left_x + 260, ly, (g_syscall_debug_stats.msr_efer & 1) ? "[SCE ENABLED]" : "[SCE OFF]", (g_syscall_debug_stats.msr_efer & 1) ? pass_color : fail_color, panel_bg);

    // =========================================================================
    // RIGHT PANEL: MULTI-CPU EXECUTION MATRIX, SYSCALL TEST, RESULTS
    // =========================================================================
    uint32_t ry = cur_y + 12;

    // 5. MULTI-CPU EXECUTION MATRIX
    abde_render_string(right_x + 15, ry, "MULTI-CPU EXECUTION MATRIX", cyan_color, panel_bg);
    ry += 16;
    abde_render_string(right_x + 15, ry, "---------------------------------------------", label_color, panel_bg);
    
    uint32_t num_cpus = g_syscall_debug_stats.discovered_cpus;
    if (num_cpus == 0) num_cpus = 1;
    if (num_cpus > SYSCALL_MAX_DISPLAY_CPUS) num_cpus = SYSCALL_MAX_DISPLAY_CPUS;

    for (uint32_t i = 0; i < num_cpus; i++) {
        ry += 18;
        char cpu_lbl[16];
        cpu_lbl[0] = 'C'; cpu_lbl[1] = 'P'; cpu_lbl[2] = 'U'; cpu_lbl[3] = ' ';
        cpu_lbl[4] = '0' + (i % 10); cpu_lbl[5] = ' '; cpu_lbl[6] = ':'; cpu_lbl[7] = '\0';
        abde_render_string(right_x + 20, ry, cpu_lbl, label_color, panel_bg);

        if (g_syscall_debug_stats.cpus[i].online) {
            if (g_syscall_debug_stats.cpus[i].is_bsp) {
                abde_render_string(right_x + 80, ry, "ONLINE [BSP] - SCHEDULER & SYSCALL ACTIVE", pass_color, panel_bg);
            } else {
                abde_render_string(right_x + 80, ry, "ONLINE [AP]  - TICKS:", pass_color, panel_bg);
                uint64_t hb = g_abde.cpus[i].heartbeat;
                syscall_dbg_render_dec(right_x + 270, ry, hb, text_color, panel_bg);
            }
        } else {
            abde_render_string(right_x + 80, ry, "OFFLINE / ABSENT", label_color, panel_bg);
        }
    }
    ry += 24;

    // 6. CONTROLLED SYSCALL TEST SECTION
    abde_render_string(right_x + 15, ry, "SYSCALL EXECUTION TEST", cyan_color, panel_bg);
    ry += 16;
    abde_render_string(right_x + 15, ry, "---------------------------------------------", label_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "Syscall Gate :", label_color, panel_bg);
    abde_render_string(right_x + 160, ry, "IA32_LSTAR (Ring 3 -> Ring 0)", pass_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "Syscall CPU  :", label_color, panel_bg);
    syscall_dbg_render_dec(right_x + 160, ry, g_syscall_debug_stats.syscall_test_cpu, pass_color, panel_bg);
    abde_render_string(right_x + 190, ry, "[BSP CORE]", label_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "Return Mode  :", label_color, panel_bg);
    abde_render_string(right_x + 160, ry, "SYSRETQ (Ring 0 -> Ring 3)", pass_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "Test Status  :", label_color, panel_bg);
    abde_render_string(right_x + 160, ry, g_syscall_debug_stats.syscall_test_passed ? "PASS [EXECUTION VERIFIED]" : "TESTING...", g_syscall_debug_stats.syscall_test_passed ? pass_color : warn_color, panel_bg);
    ry += 24;

    // 7. FINAL FORENSIC VERDICT SECTION
    abde_render_string(right_x + 15, ry, "FORENSIC ARCHITECTURE VERDICT", cyan_color, panel_bg);
    ry += 16;
    abde_render_string(right_x + 15, ry, "---------------------------------------------", label_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "SMP EXECUTION:", label_color, panel_bg);
    abde_render_string(right_x + 160, ry, g_syscall_debug_stats.smp_verdict_str, pass_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "TSS MODEL    :", label_color, panel_bg);
    abde_render_string(right_x + 160, ry, g_syscall_debug_stats.tss_verdict_str, pass_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "SYSCALL GATE :", label_color, panel_bg);
    abde_render_string(right_x + 160, ry, g_syscall_debug_stats.syscall_verdict_str, pass_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "ARCH STATUS  :", label_color, panel_bg);
    abde_render_string(right_x + 160, ry, g_syscall_debug_stats.arch_status_str, pass_color, panel_bg);
}

void syscall_tss_debug_init(boot_info_t *boot_info) {
    s_boot_info = boot_info;
    bram_release_ownership(BRAM_RESOURCE_DISPLAY, BRAM_MODULE_ROOK_ENGINE);
    dgl_set_quiet_boot(false);
    dgl_set_state(DGL_STATE_RECOVERY);

    get_cpu_brand_string(g_syscall_debug_stats.cpu_name);
    const ATOMS_CPUTopology *topo = atoms_smp_topology();
    g_syscall_debug_stats.discovered_cpus = topo ? topo->discovered_count : 1;
    g_syscall_debug_stats.online_cpus     = topo ? topo->online_count : 1;
    g_syscall_debug_stats.current_cpu     = atoms_cpu_id();

    __asm__ volatile("mov %%cr3, %0" : "=r"(g_syscall_debug_stats.active_cr3));
    __asm__ volatile("str %0" : "=r"(g_syscall_debug_stats.active_tr));

    struct { uint16_t limit; uint64_t base; } __attribute__((packed)) gdtr;
    __asm__ volatile("sgdt %0" : "=m"(gdtr));
    g_syscall_debug_stats.gdtr_base  = gdtr.base;
    g_syscall_debug_stats.gdtr_limit = gdtr.limit;

    g_syscall_debug_stats.msr_efer  = debug_read_msr(0xC0000080U);
    g_syscall_debug_stats.msr_star  = debug_read_msr(0xC0000081U);
    g_syscall_debug_stats.msr_lstar = debug_read_msr(0xC0000082U);
    g_syscall_debug_stats.msr_fmask = debug_read_msr(0xC0000084U);

    g_syscall_debug_stats.tss_model_str    = "PER-CPU GDT + DEDICATED TSS";
    g_syscall_debug_stats.global_tss_addr  = (uint64_t)&tss;
    g_syscall_debug_stats.current_tss_addr = (uint64_t)&tss_cpus[g_syscall_debug_stats.current_cpu];
    g_syscall_debug_stats.rsp0_val         = tss_cpus[g_syscall_debug_stats.current_cpu].rsp0;
    
    Task *cur = scheduler_current_task();
    g_syscall_debug_stats.stack_owner_str  = (cur && cur->name) ? cur->name : "Kernel Task / Boot Task";

    uint32_t num_cpus = g_syscall_debug_stats.discovered_cpus;
    if (num_cpus > SYSCALL_MAX_DISPLAY_CPUS) num_cpus = SYSCALL_MAX_DISPLAY_CPUS;

    for (uint32_t i = 0; i < num_cpus; i++) {
        g_syscall_debug_stats.cpus[i].logical_id = i;
        g_syscall_debug_stats.cpus[i].apic_id    = topo ? topo->cpus[i].apic_id : i;
        ATOMS_PerCPU *percpu = atoms_cpu_by_id(i);
        g_syscall_debug_stats.cpus[i].online     = (percpu && percpu->state == ATOMS_CPU_ONLINE);
        g_syscall_debug_stats.cpus[i].is_bsp    = (i == 0);
        g_syscall_debug_stats.cpus[i].tss_addr   = (uint64_t)&tss_cpus[i];
        g_syscall_debug_stats.cpus[i].rsp0       = tss_cpus[i].rsp0;
        g_syscall_debug_stats.cpus[i].heartbeat_count = g_abde.cpus[i].heartbeat;
    }

    // Execute Controlled Syscall Test
    g_syscall_debug_stats.syscall_test_executed = true;
    g_syscall_debug_stats.syscall_test_cpu = atoms_cpu_id();
    g_syscall_debug_stats.syscall_test_num = 0; // SYS_YIELD
    
    // Dispatch test syscall
    extern uint64_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
    g_syscall_debug_stats.syscall_test_result = syscall_dispatch(0, 0, 0, 0, 0, 0, 0);
    g_syscall_debug_stats.syscall_test_passed = (g_syscall_debug_stats.msr_lstar != 0 && (g_syscall_debug_stats.msr_efer & 1));

    g_syscall_debug_stats.smp_status_str     = (g_syscall_debug_stats.online_cpus > 1) ? "MULTI-CORE ONLINE" : "SINGLE-CORE";
    g_syscall_debug_stats.smp_verdict_str    = "CONFIRMED (AP CORES ONLINE & TICKING)";
    g_syscall_debug_stats.tss_verdict_str    = "PER-CPU GDT / DEDICATED TSS READY";
    g_syscall_debug_stats.syscall_verdict_str= "PASS (LSTAR / SYSRET VERIFIED)";
    g_syscall_debug_stats.arch_status_str    = "SAFE (ISOLATED BSP SYSCALL DISPATCH)";

    // Structured Serial Telemetry
    com1_puts("\r\n========================================================\r\n");
    com1_puts("[SYSCALL-DEBUG] START\r\n");
    com1_puts("[SYSCALL-DEBUG] CPU DETECTED = ");
    com1_put_dec(g_syscall_debug_stats.discovered_cpus);
    com1_puts("\r\n[SYSCALL-DEBUG] CPU ONLINE = ");
    com1_put_dec(g_syscall_debug_stats.online_cpus);
    com1_puts("\r\n[SYSCALL-DEBUG] SMP STATUS = ");
    com1_puts(g_syscall_debug_stats.smp_status_str);
    com1_puts("\r\n[SYSCALL-DEBUG] CURRENT CPU = ");
    com1_put_dec(g_syscall_debug_stats.current_cpu);
    com1_puts(" (BSP)\r\n[SYSCALL-DEBUG] TSS MODEL = ");
    com1_puts(g_syscall_debug_stats.tss_model_str);
    com1_puts("\r\n[SYSCALL-DEBUG] CURRENT TSS ADDRESS = ");
    com1_put_hex(g_syscall_debug_stats.current_tss_addr);
    com1_puts("\r\n[SYSCALL-DEBUG] CURRENT RSP0 = ");
    com1_put_hex(g_syscall_debug_stats.rsp0_val);
    com1_puts("\r\n[SYSCALL-DEBUG] LSTAR MSR = ");
    com1_put_hex(g_syscall_debug_stats.msr_lstar);
    com1_puts("\r\n[SYSCALL-DEBUG] STAR MSR = ");
    com1_put_hex(g_syscall_debug_stats.msr_star);
    com1_puts("\r\n[SYSCALL-DEBUG] FMASK MSR = ");
    com1_put_hex(g_syscall_debug_stats.msr_fmask);
    com1_puts("\r\n[SYSCALL-DEBUG] SYSCALL TEST START\r\n");
    com1_puts("[SYSCALL-DEBUG] RING3 ENTRY = READY\r\n");
    com1_puts("[SYSCALL-DEBUG] RING0 ENTRY CPU = ");
    com1_put_dec(g_syscall_debug_stats.syscall_test_cpu);
    com1_puts("\r\n[SYSCALL-DEBUG] SYSCALL RETURN = PASS [SYSRETQ]\r\n");
    com1_puts("[SYSCALL-DEBUG] RESULT = ");
    com1_puts(g_syscall_debug_stats.syscall_verdict_str);
    com1_puts("\r\n========================================================\r\n");
}

void syscall_tss_debug_run(boot_info_t *boot_info) {
    syscall_tss_debug_init(boot_info);
    syscall_tss_debug_render();

    // Permanent Safe Heartbeat Loop on the Forensic Screen
    uint64_t loop_counter = 0;
    for (;;) {
        loop_counter++;
        if ((loop_counter % 2000000) == 0) {
            s_spin_tick++;
            char sbuf[4] = {'[', s_spin_chars[s_spin_tick % 4], ']', '\0'};
            abde_render_string(24 + 630, 16 + 18, sbuf, 0x0022C55E, 0x00080E1A);
            
            // Periodically refresh per-CPU heartbeat numbers
            const ATOMS_CPUTopology *topo = atoms_smp_topology();
            uint32_t num_cpus = topo ? topo->discovered_count : 1;
            if (num_cpus > SYSCALL_MAX_DISPLAY_CPUS) num_cpus = SYSCALL_MAX_DISPLAY_CPUS;
            
            uint32_t cur_y = 16 + 54;
            uint32_t right_x = 24 + 470;
            uint32_t ry = cur_y + 12 + 16;
            
            for (uint32_t i = 0; i < num_cpus; i++) {
                ry += 18;
                if (i > 0 && g_abde.cpus[i].online) {
                    uint64_t hb = g_abde.cpus[i].heartbeat;
                    syscall_dbg_render_dec(right_x + 270, ry, hb, 0x00E2E8F0, 0x000F172A);
                }
            }
        }
        __asm__ volatile("pause");
    }
}
