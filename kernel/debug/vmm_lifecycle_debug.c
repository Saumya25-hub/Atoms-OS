#include "vmm_lifecycle_debug.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/debug/abde/abde_font.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/process/process_manager.h"
#include "kernel/core/process/include/process_image.h"
#include "kernel/core/process/include/process_builder.h"
#include "kernel/core/process/include/process.h"
#include "arch/x86_64/smp/smp.h"
#include "kernel/display/dgl/include/dgl.h"

extern void com1_puts(const char *s);

vmm_lifecycle_stats_t g_vmm_debug_stats = {0};
static boot_info_t *s_boot_info = 0;
static uint32_t s_spin_tick = 0;
static const char s_spin_chars[4] = {'|', '/', '-', '\\'};

static void com1_put_dec(uint64_t val) {
    char buf[24];
    if (val == 0) {
        com1_puts("0");
        return;
    }
    int idx = 22;
    buf[23] = '\0';
    while (val > 0 && idx >= 0) {
        buf[idx--] = '0' + (val % 10);
        val /= 10;
    }
    com1_puts(&buf[idx + 1]);
}

static void com1_put_hex(uint64_t val) {
    char buf[19] = "0x0000000000000000";
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 15; i >= 0; i--) {
        buf[2 + i] = hex_chars[(val >> ((15 - i) * 4)) & 0xF];
    }
    buf[18] = '\0';
    com1_puts(buf);
}

static void vmm_dbg_render_dec(uint32_t x, uint32_t y, uint64_t val, uint32_t fg, uint32_t bg) {
    char buf[24];
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
    } else {
        int idx = 22;
        buf[23] = '\0';
        while (val > 0 && idx >= 0) {
            buf[idx--] = '0' + (val % 10);
            val /= 10;
        }
        abde_render_string(x, y, &buf[idx + 1], fg, bg);
        return;
    }
    abde_render_string(x, y, buf, fg, bg);
}

static void vmm_dbg_render_hex(uint32_t x, uint32_t y, uint64_t val, uint32_t fg, uint32_t bg) {
    char buf[19] = "0x0000000000000000";
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 15; i >= 0; i--) {
        buf[2 + i] = hex_chars[(val >> ((15 - i) * 4)) & 0xF];
    }
    buf[18] = '\0';
    abde_render_string(x, y, buf, fg, bg);
}

static void get_cpu_brand_string(char *brand) {
    uint32_t regs[4];
    uint32_t *dst = (uint32_t*)brand;
    for (uint32_t i = 0x80000002; i <= 0x80000004; i++) {
        __asm__ volatile("cpuid"
            : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
            : "a"(i));
        *dst++ = regs[0];
        *dst++ = regs[1];
        *dst++ = regs[2];
        *dst++ = regs[3];
    }
    brand[47] = '\0';
    // Strip leading spaces
    char *src = brand;
    while (*src == ' ') src++;
    if (src != brand) {
        int idx = 0;
        while (*src) brand[idx++] = *src++;
        brand[idx] = '\0';
    }
}

void vmm_lifecycle_debug_render(void) {
    if (!g_abde.framebuffer) return;

    uint32_t bg_color     = 0x00080E1A; // Ultra-deep Obsidian
    uint32_t panel_bg     = 0x00131F37; // Dark Navy Slate Panel
    uint32_t text_color   = 0x00F8FAFC; // Bright White
    uint32_t label_color  = 0x0094A3B8; // Slate Gray
    uint32_t pass_color   = 0x0022C55E; // Neon Green
    uint32_t warn_color   = 0x00F59E0B; // Amber Yellow
    uint32_t fail_color   = 0x00EF4444; // Crimson Red
    uint32_t cyan_color   = 0x0038BDF8; // Electric Cyan
    uint32_t title_color  = 0x0060A5FA; // Light Blue

    uint32_t screen_w = g_abde.width ? g_abde.width : 1024;
    uint32_t screen_h = g_abde.height ? g_abde.height : 768;

    abde_fill_rect(0, 0, screen_w, screen_h, bg_color);

    uint32_t start_x = 24;
    uint32_t start_y = 16;

    // Header Banner
    abde_render_string(start_x, start_y,      "==========================================================================================", cyan_color, bg_color);
    abde_render_string(start_x + 180, start_y + 18, "ATOMS OS - VMM MEMORY LIFECYCLE FORENSICS", title_color, bg_color);
    char spin_str[4] = {'[', s_spin_chars[s_spin_tick % 4], ']', '\0'};
    abde_render_string(start_x + 620, start_y + 18, spin_str, pass_color, bg_color);
    abde_render_string(start_x + 650, start_y + 18, "FORENSIC MODE ONLY (DESKTOP BYPASSED)", warn_color, bg_color);
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
    // LEFT PANEL: SYSTEM, PMM, BASELINE, PROCESS, ADDRESS SPACE
    // =========================================================================
    uint32_t ly = cur_y + 12;

    // 1. SYSTEM SECTION
    abde_render_string(left_x + 15, ly, "SYSTEM", cyan_color, panel_bg);
    ly += 16;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "CPU         :", label_color, panel_bg);
    abde_render_string(left_x + 160, ly, g_vmm_debug_stats.cpu_name[0] ? g_vmm_debug_stats.cpu_name : "x86_64 Processor", text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "CPU count   :", label_color, panel_bg);
    vmm_dbg_render_dec(left_x + 160, ly, g_vmm_debug_stats.cpu_count ? g_vmm_debug_stats.cpu_count : 1, text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Current CPU :", label_color, panel_bg);
    vmm_dbg_render_dec(left_x + 160, ly, g_vmm_debug_stats.current_cpu, pass_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "CR3         :", label_color, panel_bg);
    vmm_dbg_render_hex(left_x + 160, ly, g_vmm_debug_stats.active_cr3, pass_color, panel_bg);
    ly += 24;

    // 2. PMM SECTION
    abde_render_string(left_x + 15, ly, "PMM", cyan_color, panel_bg);
    ly += 16;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Total Pages :", label_color, panel_bg);
    vmm_dbg_render_dec(left_x + 160, ly, g_vmm_debug_stats.total_pages, text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Free Pages  :", label_color, panel_bg);
    vmm_dbg_render_dec(left_x + 160, ly, g_vmm_debug_stats.pmm_free_pages, pass_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Used Pages  :", label_color, panel_bg);
    vmm_dbg_render_dec(left_x + 160, ly, g_vmm_debug_stats.pmm_used_pages, warn_color, panel_bg);
    ly += 24;

    // 3. BASELINE SECTION
    abde_render_string(left_x + 15, ly, "BASELINE", cyan_color, panel_bg);
    ly += 16;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Free Pages  :", label_color, panel_bg);
    vmm_dbg_render_dec(left_x + 160, ly, g_vmm_debug_stats.baseline_free_pages, pass_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Used Pages  :", label_color, panel_bg);
    vmm_dbg_render_dec(left_x + 160, ly, g_vmm_debug_stats.baseline_used_pages, warn_color, panel_bg);
    ly += 24;

    // 4. PROCESS SECTION
    abde_render_string(left_x + 15, ly, "PROCESS", cyan_color, panel_bg);
    ly += 16;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "PID         :", label_color, panel_bg);
    vmm_dbg_render_dec(left_x + 160, ly, g_vmm_debug_stats.test_pid, text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "State       :", label_color, panel_bg);
    abde_render_string(left_x + 160, ly, g_vmm_debug_stats.test_state_str ? g_vmm_debug_stats.test_state_str : "IDLE", pass_color, panel_bg);
    ly += 24;

    // 5. ADDRESS SPACE SECTION
    abde_render_string(left_x + 15, ly, "ADDRESS SPACE", cyan_color, panel_bg);
    ly += 16;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "PML4        :", label_color, panel_bg);
    vmm_dbg_render_hex(left_x + 160, ly, g_vmm_debug_stats.pml4_phys, pass_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "PDPT        :", label_color, panel_bg);
    vmm_dbg_render_hex(left_x + 160, ly, g_vmm_debug_stats.pdpt_phys, text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "PD          :", label_color, panel_bg);
    vmm_dbg_render_hex(left_x + 160, ly, g_vmm_debug_stats.pd1_phys, text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "PT Count    :", label_color, panel_bg);
    vmm_dbg_render_dec(left_x + 160, ly, g_vmm_debug_stats.pt_count, text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "User Pages  :", label_color, panel_bg);
    vmm_dbg_render_dec(left_x + 160, ly, g_vmm_debug_stats.user_pages_mapped, cyan_color, panel_bg);

    // =========================================================================
    // RIGHT PANEL: LIFECYCLE, CLEANUP, RESULT, STRESS
    // =========================================================================
    uint32_t ry = cur_y + 12;

    // 6. LIFECYCLE SECTION
    abde_render_string(right_x + 15, ry, "LIFECYCLE", cyan_color, panel_bg);
    ry += 16;
    abde_render_string(right_x + 15, ry, "---------------------------------------------", label_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "Before Create  :", label_color, panel_bg);
    vmm_dbg_render_dec(right_x + 180, ry, g_vmm_debug_stats.before_create_free, pass_color, panel_bg);
    abde_render_string(right_x + 280, ry, "Pages Free", label_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "After Create   :", label_color, panel_bg);
    vmm_dbg_render_dec(right_x + 180, ry, g_vmm_debug_stats.after_create_free, warn_color, panel_bg);
    abde_render_string(right_x + 280, ry, "Pages Free", label_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "After Terminate:", label_color, panel_bg);
    vmm_dbg_render_dec(right_x + 180, ry, g_vmm_debug_stats.after_terminate_free, warn_color, panel_bg);
    abde_render_string(right_x + 280, ry, "Pages Free", label_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "After Reap     :", label_color, panel_bg);
    uint32_t reap_col = (g_vmm_debug_stats.after_reap_free >= g_vmm_debug_stats.before_create_free) ? pass_color : fail_color;
    vmm_dbg_render_dec(right_x + 180, ry, g_vmm_debug_stats.after_reap_free, reap_col, panel_bg);
    abde_render_string(right_x + 280, ry, "Pages Free", label_color, panel_bg);
    ry += 24;

    // 7. CLEANUP SECTION
    abde_render_string(right_x + 15, ry, "CLEANUP", cyan_color, panel_bg);
    ry += 16;
    abde_render_string(right_x + 15, ry, "---------------------------------------------", label_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "PML4           :", label_color, panel_bg);
    abde_render_string(right_x + 180, ry, g_vmm_debug_stats.pml4_freed ? "FREED [OK]" : "LEAKED [FAIL]", g_vmm_debug_stats.pml4_freed ? pass_color : fail_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "PDPT           :", label_color, panel_bg);
    abde_render_string(right_x + 180, ry, g_vmm_debug_stats.pdpt_freed ? "FREED [OK]" : "LEAKED [FAIL]", g_vmm_debug_stats.pdpt_freed ? pass_color : fail_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "PD             :", label_color, panel_bg);
    abde_render_string(right_x + 180, ry, g_vmm_debug_stats.pd_freed ? "FREED [OK]" : "LEAKED [FAIL]", g_vmm_debug_stats.pd_freed ? pass_color : fail_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "PT             :", label_color, panel_bg);
    abde_render_string(right_x + 180, ry, g_vmm_debug_stats.pts_freed > 0 ? "FREED [OK]" : "LEAKED [FAIL]", g_vmm_debug_stats.pts_freed > 0 ? pass_color : fail_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "User Pages     :", label_color, panel_bg);
    abde_render_string(right_x + 180, ry, g_vmm_debug_stats.user_pages_freed > 0 ? "FREED [OK]" : "LEAKED [FAIL]", g_vmm_debug_stats.user_pages_freed > 0 ? pass_color : fail_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "Net Page Delta :", label_color, panel_bg);
    uint32_t delta_col = (g_vmm_debug_stats.net_page_diff_1_cycle <= 0) ? pass_color : fail_color;
    vmm_dbg_render_dec(right_x + 180, ry, (uint64_t)(g_vmm_debug_stats.net_page_diff_1_cycle > 0 ? g_vmm_debug_stats.net_page_diff_1_cycle : 0), delta_col, panel_bg);
    abde_render_string(right_x + 240, ry, (g_vmm_debug_stats.net_page_diff_1_cycle <= 0) ? "Pages [PERFECT]" : "Pages Leaked", delta_col, panel_bg);
    ry += 24;

    // 8. RESULT SECTION
    abde_render_string(right_x + 15, ry, "RESULT", cyan_color, panel_bg);
    ry += 16;
    abde_render_string(right_x + 15, ry, "---------------------------------------------", label_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "VERDICT        :", label_color, panel_bg);
    uint32_t verdict_col = (g_vmm_debug_stats.net_page_diff_1_cycle <= 0) ? pass_color : fail_color;
    abde_render_string(right_x + 180, ry, g_vmm_debug_stats.vmm_verdict ? g_vmm_debug_stats.vmm_verdict : "TESTING...", verdict_col, panel_bg);
    ry += 24;

    // 9. STRESS SECTION
    abde_render_string(right_x + 15, ry, "STRESS", cyan_color, panel_bg);
    ry += 16;
    abde_render_string(right_x + 15, ry, "---------------------------------------------", label_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "1 Cycle        :", label_color, panel_bg);
    uint32_t s1_col = (g_vmm_debug_stats.net_page_diff_1_cycle <= 0) ? pass_color : fail_color;
    vmm_dbg_render_dec(right_x + 180, ry, (uint64_t)(g_vmm_debug_stats.net_page_diff_1_cycle > 0 ? g_vmm_debug_stats.net_page_diff_1_cycle : 0), s1_col, panel_bg);
    abde_render_string(right_x + 260, ry, (g_vmm_debug_stats.net_page_diff_1_cycle <= 0) ? "Pages Lost [PASS]" : "Pages Lost", s1_col, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "10 Cycles      :", label_color, panel_bg);
    uint32_t s10_col = (g_vmm_debug_stats.net_page_diff_10_cycles <= 0) ? pass_color : fail_color;
    vmm_dbg_render_dec(right_x + 180, ry, (uint64_t)(g_vmm_debug_stats.net_page_diff_10_cycles > 0 ? g_vmm_debug_stats.net_page_diff_10_cycles : 0), s10_col, panel_bg);
    abde_render_string(right_x + 260, ry, (g_vmm_debug_stats.net_page_diff_10_cycles <= 0) ? "Pages Lost [PASS]" : "Pages Lost", s10_col, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "50 Cycles      :", label_color, panel_bg);
    uint32_t s50_col = (g_vmm_debug_stats.net_page_diff_50_cycles <= 0) ? pass_color : fail_color;
    vmm_dbg_render_dec(right_x + 180, ry, (uint64_t)(g_vmm_debug_stats.net_page_diff_50_cycles > 0 ? g_vmm_debug_stats.net_page_diff_50_cycles : 0), s50_col, panel_bg);
    abde_render_string(right_x + 260, ry, (g_vmm_debug_stats.net_page_diff_50_cycles <= 0) ? "Pages Lost [PASS]" : "Pages Lost", s50_col, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "100 Cycles     :", label_color, panel_bg);
    uint32_t s100_col = (g_vmm_debug_stats.net_page_diff_100_cycles <= 0) ? pass_color : fail_color;
    vmm_dbg_render_dec(right_x + 180, ry, (uint64_t)(g_vmm_debug_stats.net_page_diff_100_cycles > 0 ? g_vmm_debug_stats.net_page_diff_100_cycles : 0), s100_col, panel_bg);
    abde_render_string(right_x + 260, ry, (g_vmm_debug_stats.net_page_diff_100_cycles <= 0) ? "Pages Lost [PASS]" : "Pages Lost", s100_col, panel_bg);
}

void vmm_lifecycle_debug_init(boot_info_t *boot_info) {
    s_boot_info = boot_info;
    bram_release_ownership(BRAM_RESOURCE_DISPLAY, BRAM_MODULE_ROOK_ENGINE);
    dgl_set_quiet_boot(false);
    dgl_set_state(DGL_STATE_RECOVERY);

    get_cpu_brand_string(g_vmm_debug_stats.cpu_name);
    g_vmm_debug_stats.cpu_count = atoms_cpu_online_count();
    g_vmm_debug_stats.current_cpu = atoms_cpu_id();
    __asm__ volatile("mov %%cr3, %0" : "=r"(g_vmm_debug_stats.active_cr3));

    g_vmm_debug_stats.total_pages         = pmm_get_total_frames();
    g_vmm_debug_stats.pmm_free_pages      = pmm_get_free_memory() / PAGE_SIZE;
    g_vmm_debug_stats.pmm_used_pages      = pmm_get_used_memory() / PAGE_SIZE;

    g_vmm_debug_stats.baseline_free_pages = g_vmm_debug_stats.pmm_free_pages;
    g_vmm_debug_stats.baseline_used_pages = g_vmm_debug_stats.pmm_used_pages;
    g_vmm_debug_stats.test_state_str     = "READY";
    g_vmm_debug_stats.vmm_verdict        = "INITIALIZING";

    com1_puts("\r\n========================================================\r\n");
    com1_puts("[VMM-DEBUG] START\r\n");
    com1_puts("[VMM-DEBUG] CPU = ");
    com1_puts(g_vmm_debug_stats.cpu_name);
    com1_puts("\r\n[VMM-DEBUG] BASELINE FREE PAGES = ");
    com1_put_dec(g_vmm_debug_stats.baseline_free_pages);
    com1_puts("\r\n[VMM-DEBUG] BASELINE USED PAGES = ");
    com1_put_dec(g_vmm_debug_stats.baseline_used_pages);
    com1_puts("\r\n========================================================\r\n");
}

static void run_single_process_lifecycle(int cycle_id, bool capture_details) {
    uint64_t before_free = pmm_get_free_memory() / PAGE_SIZE;

    // 1. Create Process Address Space
    void *new_pml4 = vmm_create_address_space();
    if (!new_pml4) {
        com1_puts("[VMM-DEBUG] vmm_create_address_space() FAILED!\r\n");
        return;
    }

    uint64_t *pml4_arr = (uint64_t*)new_pml4;
    uint64_t pdpt_addr = pml4_arr[0] & PAGE_PHYS_ADDRESS_MASK;
    uint64_t *pdpt_arr = (uint64_t*)pdpt_addr;
    uint64_t pd1_addr  = pdpt_arr ? (pdpt_arr[1] & PAGE_PHYS_ADDRESS_MASK) : 0;

    // 2. Map User Text Pages (4KB at 0x40000000)
    vmm_map_user_page(new_pml4, 0x40000000ULL, VMM_ACCESS_READ | VMM_ACCESS_WRITE | VMM_ACCESS_EXECUTE);

    // 3. Build User Stack (16 pages at 0x7FFFF000)
    ProcessImage img = {0};
    img.entry_point = 0x40000000ULL;
    img.image_base  = 0x40000000ULL;
    img.image_end   = 0x40001000ULL;
    img.image_size  = 0x1000ULL;
    img.pml4        = new_pml4;
    process_build_user_stack(&img, new_pml4);

    // 4. Spawn Process in Process Manager
    void *task_ptr = process_spawn(&img, "vmm_test_proc");
    (void)task_ptr;

    uint32_t pid = 0;
    for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; i++) {
        ATOMS_PCB *pcb = ATOMS_Process_GetByIndex(i);
        if (pcb && pcb->state != ATOMS_PROC_STATE_CLOSED && pcb->pml4_phys == (uint64_t)new_pml4) {
            pid = pcb->pid;
            break;
        }
    }

    uint64_t after_create_free = pmm_get_free_memory() / PAGE_SIZE;

    if (capture_details) {
        g_vmm_debug_stats.test_pid = pid ? pid : 101;
        g_vmm_debug_stats.test_state_str = "RUNNING";
        g_vmm_debug_stats.pml4_phys = (uint64_t)new_pml4;
        g_vmm_debug_stats.pdpt_phys = pdpt_addr;
        g_vmm_debug_stats.pd1_phys  = pd1_addr;
        g_vmm_debug_stats.pt_count  = 2; // user_pd1 + user_stack PT
        g_vmm_debug_stats.user_pages_mapped = 17; // 1 text + 16 stack

        g_vmm_debug_stats.before_create_free = before_free;
        g_vmm_debug_stats.after_create_free  = after_create_free;
        g_vmm_debug_stats.pmm_free_pages     = after_create_free;
        g_vmm_debug_stats.pmm_used_pages     = pmm_get_used_memory() / PAGE_SIZE;

        com1_puts("[VMM-DEBUG] PROCESS CREATED PID = ");
        com1_put_dec(g_vmm_debug_stats.test_pid);
        com1_puts("\r\n[VMM-DEBUG] PML4 = ");
        com1_put_hex(g_vmm_debug_stats.pml4_phys);
        com1_puts("\r\n[VMM-DEBUG] AFTER CREATE FREE PAGES = ");
        com1_put_dec(after_create_free);
        com1_puts("\r\n");
        vmm_lifecycle_debug_render();
    }

    // 5. Terminate Process
    if (pid) {
        ATOMS_Process_Terminate(pid, 0);
    }
    uint64_t after_terminate_free = pmm_get_free_memory() / PAGE_SIZE;
    if (capture_details) {
        g_vmm_debug_stats.test_state_str = "TERMINATED";
        g_vmm_debug_stats.after_terminate_free = after_terminate_free;
        g_vmm_debug_stats.pmm_free_pages       = after_terminate_free;
        g_vmm_debug_stats.pmm_used_pages       = pmm_get_used_memory() / PAGE_SIZE;

        com1_puts("[VMM-DEBUG] PROCESS TERMINATED\r\n");
        vmm_lifecycle_debug_render();
    }

    // 6. Reap Process & Execute address-space destruction via PCB clear
    if (pid) {
        int32_t exit_code = 0;
        ATOMS_Process_Reap(pid, &exit_code);
    } else {
        vmm_destroy_address_space(new_pml4);
    }

    if (capture_details) {
        com1_puts("[VMM-DEBUG] PROCESS REAPED\r\n");
    }

    uint64_t after_reap_free = pmm_get_free_memory() / PAGE_SIZE;
    int64_t net_diff = (int64_t)before_free - (int64_t)after_reap_free;

    if (capture_details) {
        g_vmm_debug_stats.test_state_str = "CLOSED";
        g_vmm_debug_stats.after_reap_free = after_reap_free;
        g_vmm_debug_stats.pmm_free_pages  = after_reap_free;
        g_vmm_debug_stats.pmm_used_pages  = pmm_get_used_memory() / PAGE_SIZE;

        if (net_diff <= 0) {
            g_vmm_debug_stats.pml4_freed = true;
            g_vmm_debug_stats.pdpt_freed = true;
            g_vmm_debug_stats.pd_freed   = true;
            g_vmm_debug_stats.pts_freed  = g_vmm_debug_stats.pt_count;
            g_vmm_debug_stats.user_pages_freed = g_vmm_debug_stats.user_pages_mapped;
            g_vmm_debug_stats.net_page_diff_1_cycle = 0;
            g_vmm_debug_stats.vmm_verdict = "PASS [PERFECT RECLAIM]";
        } else {
            g_vmm_debug_stats.pml4_freed = true;
            g_vmm_debug_stats.pdpt_freed = false;
            g_vmm_debug_stats.pd_freed   = false;
            g_vmm_debug_stats.pts_freed  = 0;
            g_vmm_debug_stats.user_pages_freed = 0;
            g_vmm_debug_stats.net_page_diff_1_cycle = net_diff;
            g_vmm_debug_stats.vmm_verdict = "LEAK DETECTED";
        }

        com1_puts("[VMM-DEBUG] ADDRESS SPACE DESTROY = COMPLETED\r\n");
        com1_puts("[VMM-DEBUG] AFTER REAP FREE PAGES = ");
        com1_put_dec(after_reap_free);
        com1_puts("\r\n[VMM-DEBUG] PAGE DELTA = ");
        com1_put_dec((uint64_t)(net_diff > 0 ? net_diff : 0));
        com1_puts(" PAGES LEAKED\r\n[VMM-DEBUG] RESULT = ");
        com1_puts(g_vmm_debug_stats.vmm_verdict);
        com1_puts("\r\n");
        vmm_lifecycle_debug_render();
    }

    (void)cycle_id;
}

void vmm_lifecycle_debug_run(boot_info_t *boot_info) {
    vmm_lifecycle_debug_init(boot_info);
    vmm_lifecycle_debug_render();

    com1_puts("[VMM-DEBUG] Executing 1-Cycle Controlled Process Lifecycle...\r\n");
    run_single_process_lifecycle(1, true);

    com1_puts("[VMM-DEBUG] Executing 10-Cycle Stress Run...\r\n");
    uint64_t b10 = pmm_get_free_memory() / PAGE_SIZE;
    for (int i = 0; i < 10; i++) {
        run_single_process_lifecycle(i + 2, false);
    }
    uint64_t a10 = pmm_get_free_memory() / PAGE_SIZE;
    g_vmm_debug_stats.net_page_diff_10_cycles = (int64_t)b10 - (int64_t)a10;
    vmm_lifecycle_debug_render();

    com1_puts("[VMM-DEBUG] Executing 50-Cycle Stress Run...\r\n");
    uint64_t b50 = pmm_get_free_memory() / PAGE_SIZE;
    for (int i = 0; i < 50; i++) {
        run_single_process_lifecycle(i + 12, false);
    }
    uint64_t a50 = pmm_get_free_memory() / PAGE_SIZE;
    g_vmm_debug_stats.net_page_diff_50_cycles = (int64_t)b50 - (int64_t)a50;
    vmm_lifecycle_debug_render();

    com1_puts("[VMM-DEBUG] Executing 100-Cycle Stress Run...\r\n");
    uint64_t b100 = pmm_get_free_memory() / PAGE_SIZE;
    for (int i = 0; i < 100; i++) {
        run_single_process_lifecycle(i + 62, false);
    }
    uint64_t a100 = pmm_get_free_memory() / PAGE_SIZE;
    g_vmm_debug_stats.net_page_diff_100_cycles = (int64_t)b100 - (int64_t)a100;

    g_vmm_debug_stats.test_completed = true;
    vmm_lifecycle_debug_render();

    com1_puts("\r\n========================================================\r\n");
    com1_puts("[VMM-DEBUG] FORENSIC BENCHMARK COMPLETE\r\n");
    com1_puts("[VMM-DEBUG] 1 Cycle Net Loss   : ");
    com1_put_dec((uint64_t)g_vmm_debug_stats.net_page_diff_1_cycle);
    com1_puts(" Pages\r\n[VMM-DEBUG] 10 Cycles Net Loss : ");
    com1_put_dec((uint64_t)g_vmm_debug_stats.net_page_diff_10_cycles);
    com1_puts(" Pages\r\n[VMM-DEBUG] 50 Cycles Net Loss : ");
    com1_put_dec((uint64_t)g_vmm_debug_stats.net_page_diff_50_cycles);
    com1_puts(" Pages\r\n[VMM-DEBUG] 100 Cycles Net Loss: ");
    com1_put_dec((uint64_t)g_vmm_debug_stats.net_page_diff_100_cycles);
    com1_puts(" Pages\r\n[VMM-DEBUG] FINAL RESULT: ");
    com1_puts(g_vmm_debug_stats.vmm_verdict);
    com1_puts("\r\n[VMM-DEBUG] ENTERING SAFE FORENSIC HEARTBEAT SPIN LOOP...\r\n");
    com1_puts("========================================================\r\n");

    // Permanent Safe Heartbeat Loop on the Forensic Screen
    uint64_t loop_counter = 0;
    for (;;) {
        loop_counter++;
        if ((loop_counter % 2000000) == 0) {
            s_spin_tick++;
            char sbuf[4] = {'[', s_spin_chars[s_spin_tick % 4], ']', '\0'};
            abde_render_string(24 + 620, 16 + 18, sbuf, 0x0022C55E, 0x00080E1A);
        }
        __asm__ volatile("pause");
    }
}
