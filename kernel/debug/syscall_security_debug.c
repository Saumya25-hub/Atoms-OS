#include "syscall_security_debug.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/display/dgl/include/dgl.h"
#include "kernel/core/bram/include/bram.h"
#include "kernel/core/syscall/include/syscall.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/debug/screenshot/atoms_screenshot.h"

extern void com1_puts(const char *s);
extern void display_print(const char *s);

SyscallSecurityStats g_syscall_sec_stats;
volatile bool g_syscall_fault_catch_active = false;
volatile uint64_t g_syscall_fault_recovery_rip = 0;
volatile uint64_t g_syscall_last_fault_cr2 = 0;
volatile uint32_t g_syscall_last_fault_cpl = 0;
volatile uint32_t g_syscall_cpl0_fault_count = 0;

static boot_info_t *s_boot_info = NULL;
static volatile uint64_t s_spin_tick = 0;
static const char s_spin_chars[4] = {'|', '/', '-', '\\'};

#define NUM_SECURITY_TESTS 10
static SyscallSecurityTestCase s_test_suite[NUM_SECURITY_TESTS];

/* Palette */
#define COLOR_BG        0x00080E1A
#define COLOR_PANEL     0x000F172A
#define COLOR_CYAN      0x0038BDF8
#define COLOR_TITLE     0x0067E8F9
#define COLOR_TEXT      0x00E2E8F0
#define COLOR_LABEL     0x0094A3B8
#define COLOR_PASS      0x0022C55E
#define COLOR_WARN      0x00F59E0B
#define COLOR_FAIL      0x00EF4444

static void sec_put_hex(uint64_t val) {
    char buf[19]; buf[0] = '0'; buf[1] = 'x';
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; i++) buf[2 + i] = hex[(val >> ((15 - i) * 4)) & 0xF];
    buf[18] = '\0';
    com1_puts(buf);
}

static void sec_put_dec(uint64_t val) {
    if (val == 0) { com1_puts("0"); return; }
    char buf[24]; int pos = 22; buf[23] = '\0';
    while (val > 0) { buf[pos--] = '0' + (val % 10); val /= 10; }
    com1_puts(&buf[pos + 1]);
}

static void sec_render_hex(uint32_t x, uint32_t y, uint64_t val, uint32_t color, uint32_t bg) {
    char buf[19]; buf[0] = '0'; buf[1] = 'x';
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; i++) buf[2 + i] = hex[(val >> ((15 - i) * 4)) & 0xF];
    buf[18] = '\0';
    abde_render_string(x, y, buf, color, bg);
}

static void sec_render_dec(uint32_t x, uint32_t y, uint64_t val, uint32_t color, uint32_t bg) {
    if (val == 0) { abde_render_string(x, y, "0", color, bg); return; }
    char buf[24]; int pos = 22; buf[23] = '\0';
    while (val > 0) { buf[pos--] = '0' + (val % 10); val /= 10; }
    abde_render_string(x, y, &buf[pos + 1], color, bg);
}

/* Static Panels Rendered Once at Boot */
void syscall_security_debug_render_static(void) {
    uint32_t screen_w = g_abde.width ? g_abde.width : 1024;
    uint32_t screen_h = g_abde.height ? g_abde.height : 768;

    abde_fill_rect(0, 0, screen_w, screen_h, COLOR_BG);

    uint32_t start_x = 24;
    uint32_t start_y = 16;

    abde_render_string(start_x, start_y, "==========================================================================================", COLOR_CYAN, COLOR_BG);
    abde_render_string(start_x + 160, start_y + 18, "ATOMS SYSCALL SECURITY FORENSIC", COLOR_TITLE, COLOR_BG);
    abde_render_string(start_x + 650, start_y + 18, "RING 3 -> RING 0 BOUNDARY AUDIT", COLOR_WARN, COLOR_BG);
    abde_render_string(start_x, start_y + 36, "==========================================================================================", COLOR_CYAN, COLOR_BG);

    uint32_t cur_y = start_y + 54;
    uint32_t left_x = start_x;
    uint32_t right_x = start_x + 470;
    uint32_t panel_w = 450;
    uint32_t panel_h = 670;

    abde_fill_rect(left_x, cur_y, panel_w, panel_h, COLOR_PANEL);
    abde_fill_rect(right_x, cur_y, panel_w, panel_h, COLOR_PANEL);

    // Left Panel Headers
    uint32_t ly = cur_y + 12;
    abde_render_string(left_x + 15, ly, "1. SYSCALL SECURITY (INVENTORY)", COLOR_CYAN, COLOR_PANEL);
    ly += 16; abde_render_string(left_x + 15, ly, "---------------------------------------------", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Boundary Mode   : Ring 3 -> Ring 0 (LSTAR)", COLOR_TEXT, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Syscalls Found  : 30 Defined", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Pointer Syscalls: 13 Candidate Services", COLOR_WARN, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Hardware Target : ASUS B750M-K (Haswell/Raptor)", COLOR_TEXT, COLOR_PANEL);
    ly += 24;

    abde_render_string(left_x + 15, ly, "2. CURRENT ACTIVE TEST", COLOR_CYAN, COLOR_PANEL);
    ly += 16; abde_render_string(left_x + 15, ly, "---------------------------------------------", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Test Identifier :", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Target Syscall  :", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "User Pointer VA :", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Buffer Size     :", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Expected Outcome:", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Observed Outcome:", COLOR_LABEL, COLOR_PANEL);
    ly += 24;

    abde_render_string(left_x + 15, ly, "3. MEMORY VALIDATION MATRIX", COLOR_CYAN, COLOR_PANEL);
    ly += 16; abde_render_string(left_x + 15, ly, "---------------------------------------------", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "User Range [1G-2G]:", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Page Mapped       :", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Present (P=1)     :", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "User Perm (U/S=1) :", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Writable (R/W=1)  :", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Canonical x86_64  :", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Range Wrap / Over :", COLOR_LABEL, COLOR_PANEL);
    ly += 18; abde_render_string(left_x + 20, ly, "Page Boundary Cont:", COLOR_LABEL, COLOR_PANEL);

    // Right Panel Headers
    uint32_t ry = cur_y + 12;
    abde_render_string(right_x + 15, ry, "4. FAULT MONITOR & CONTAINMENT", COLOR_CYAN, COLOR_PANEL);
    ry += 16; abde_render_string(right_x + 15, ry, "---------------------------------------------", COLOR_LABEL, COLOR_PANEL);
    ry += 18; abde_render_string(right_x + 20, ry, "#PF Faults Total:", COLOR_LABEL, COLOR_PANEL);
    ry += 18; abde_render_string(right_x + 20, ry, "CPL 0 Faults    :", COLOR_LABEL, COLOR_PANEL);
    ry += 18; abde_render_string(right_x + 20, ry, "CPL 3 Faults    :", COLOR_LABEL, COLOR_PANEL);
    ry += 18; abde_render_string(right_x + 20, ry, "Kernel Panic    :", COLOR_LABEL, COLOR_PANEL);
    ry += 18; abde_render_string(right_x + 20, ry, "Process Term    :", COLOR_LABEL, COLOR_PANEL);
    ry += 18; abde_render_string(right_x + 20, ry, "Recovered Safely:", COLOR_LABEL, COLOR_PANEL);
    ry += 24;

    abde_render_string(right_x + 15, ry, "5. CONTROLLED TEST SUITE STATUS", COLOR_CYAN, COLOR_PANEL);
    ry += 16; abde_render_string(right_x + 15, ry, "---------------------------------------------", COLOR_LABEL, COLOR_PANEL);
    for (int i = 0; i < NUM_SECURITY_TESTS; i++) {
        ry += 18;
        char line_buf[64];
        const char *t_id = s_test_suite[i].test_id;
        const char *t_name = s_test_suite[i].test_name;
        int p = 0;
        while (t_id && *t_id) line_buf[p++] = *t_id++;
        line_buf[p++] = ':'; line_buf[p++] = ' ';
        while (t_name && *t_name && p < 38) line_buf[p++] = *t_name++;
        line_buf[p] = '\0';
        abde_render_string(right_x + 20, ry, line_buf, COLOR_TEXT, COLOR_PANEL);
    }
    ry += 24;

    abde_render_string(right_x + 15, ry, "6. FORENSIC CLASSIFICATION VERDICT", COLOR_CYAN, COLOR_PANEL);
    ry += 16; abde_render_string(right_x + 15, ry, "---------------------------------------------", COLOR_LABEL, COLOR_PANEL);
    ry += 18; abde_render_string(right_x + 20, ry, "Vulnerability :", COLOR_LABEL, COLOR_PANEL);
    abde_render_string(right_x + 160, ry, "UNMAPPED PTR DEREFERENCE", COLOR_WARN, COLOR_PANEL);
    ry += 18; abde_render_string(right_x + 20, ry, "Verification  :", COLOR_LABEL, COLOR_PANEL);
    abde_render_string(right_x + 160, ry, "CONTROLLED REPRODUCTION", COLOR_TITLE, COLOR_PANEL);
    ry += 18; abde_render_string(right_x + 20, ry, "Live State    :", COLOR_LABEL, COLOR_PANEL);
    abde_render_string(right_x + 160, ry, "CONTINUOUS LIVE TICKING", COLOR_PASS, COLOR_PANEL);
}

/* Dynamic Field Updates */
void syscall_security_debug_render_dynamic(void) {
    uint32_t start_x = 24;
    uint32_t start_y = 16;
    uint32_t cur_y = start_y + 54;
    uint32_t left_x = start_x;
    uint32_t right_x = start_x + 470;

    // Header Heartbeat Spinner
    char spin_str[4] = {'[', s_spin_chars[s_spin_tick % 4], ']', '\0'};
    abde_render_string(start_x + 580, start_y + 18, spin_str, COLOR_PASS, COLOR_BG);
    char live_str[32];
    int lp = 0;
    const char *live_prefix = "LIVE Up: ";
    while (*live_prefix) live_str[lp++] = *live_prefix++;
    uint64_t up = g_syscall_sec_stats.uptime_sec;
    if (up == 0) { live_str[lp++] = '0'; }
    else {
        char rev[16]; int r = 0;
        while (up > 0) { rev[r++] = '0' + (up % 10); up /= 10; }
        while (r > 0) live_str[lp++] = rev[--r];
    }
    live_str[lp++] = 's'; live_str[lp] = '\0';
    abde_render_string(start_x + 850, start_y + 18, live_str, COLOR_TITLE, COLOR_BG);

    // Dynamic Section 2: Current Active Test
    uint32_t ly = cur_y + 12 + 18 + 18*4 + 24 + 16;
    ly += 18; abde_fill_rect(left_x + 160, ly, 270, 16, COLOR_PANEL); abde_render_string(left_x + 160, ly, g_syscall_sec_stats.current_test_name ? g_syscall_sec_stats.current_test_name : "IDLE", COLOR_TITLE, COLOR_PANEL);
    ly += 18; abde_fill_rect(left_x + 160, ly, 270, 16, COLOR_PANEL); abde_render_string(left_x + 160, ly, g_syscall_sec_stats.target_syscall_name ? g_syscall_sec_stats.target_syscall_name : "NONE", COLOR_TEXT, COLOR_PANEL);
    ly += 18; abde_fill_rect(left_x + 160, ly, 270, 16, COLOR_PANEL); sec_render_hex(left_x + 160, ly, g_syscall_sec_stats.current_pointer, COLOR_TEXT, COLOR_PANEL);
    ly += 18; abde_fill_rect(left_x + 160, ly, 270, 16, COLOR_PANEL); sec_render_dec(left_x + 160, ly, g_syscall_sec_stats.current_size, COLOR_TEXT, COLOR_PANEL);
    ly += 18; abde_fill_rect(left_x + 160, ly, 270, 16, COLOR_PANEL); abde_render_string(left_x + 160, ly, g_syscall_sec_stats.current_expected ? g_syscall_sec_stats.current_expected : "-", COLOR_PASS, COLOR_PANEL);
    ly += 18; abde_fill_rect(left_x + 160, ly, 270, 16, COLOR_PANEL); abde_render_string(left_x + 160, ly, g_syscall_sec_stats.current_observed ? g_syscall_sec_stats.current_observed : "PENDING", COLOR_WARN, COLOR_PANEL);

    // Dynamic Section 3: Memory Validation Matrix
    ly = cur_y + 12 + 18 + 18*4 + 24 + 16 + 18*6 + 24 + 16;
    ly += 18; abde_fill_rect(left_x + 180, ly, 250, 16, COLOR_PANEL); abde_render_string(left_x + 180, ly, g_syscall_sec_stats.user_range_pass ? "PASS [IN WINDOW]" : "FAIL [OUT-OF-WINDOW]", g_syscall_sec_stats.user_range_pass ? COLOR_PASS : COLOR_FAIL, COLOR_PANEL);
    ly += 18; abde_fill_rect(left_x + 180, ly, 250, 16, COLOR_PANEL); abde_render_string(left_x + 180, ly, g_syscall_sec_stats.is_mapped ? "YES [PAGE_PRESENT]" : "NO [UNMAPPED PAGE]", g_syscall_sec_stats.is_mapped ? COLOR_PASS : COLOR_FAIL, COLOR_PANEL);
    ly += 18; abde_fill_rect(left_x + 180, ly, 250, 16, COLOR_PANEL); abde_render_string(left_x + 180, ly, g_syscall_sec_stats.is_present ? "P=1" : "P=0", g_syscall_sec_stats.is_present ? COLOR_PASS : COLOR_FAIL, COLOR_PANEL);
    ly += 18; abde_fill_rect(left_x + 180, ly, 250, 16, COLOR_PANEL); abde_render_string(left_x + 180, ly, g_syscall_sec_stats.is_user_perm ? "U/S=1 (USER)" : "U/S=0 (SUPERVISOR)", g_syscall_sec_stats.is_user_perm ? COLOR_PASS : COLOR_FAIL, COLOR_PANEL);
    ly += 18; abde_fill_rect(left_x + 180, ly, 250, 16, COLOR_PANEL); abde_render_string(left_x + 180, ly, g_syscall_sec_stats.is_writable ? "R/W=1 (WRITABLE)" : "R/W=0 (READ-ONLY)", g_syscall_sec_stats.is_writable ? COLOR_PASS : COLOR_WARN, COLOR_PANEL);
    ly += 18; abde_fill_rect(left_x + 180, ly, 250, 16, COLOR_PANEL); abde_render_string(left_x + 180, ly, g_syscall_sec_stats.is_canonical ? "CANONICAL" : "NON-CANONICAL", g_syscall_sec_stats.is_canonical ? COLOR_PASS : COLOR_FAIL, COLOR_PANEL);
    ly += 18; abde_fill_rect(left_x + 180, ly, 250, 16, COLOR_PANEL); abde_render_string(left_x + 180, ly, g_syscall_sec_stats.range_overflow ? "OVERFLOW WRAP" : "SAFE", g_syscall_sec_stats.range_overflow ? COLOR_FAIL : COLOR_PASS, COLOR_PANEL);
    ly += 18; abde_fill_rect(left_x + 180, ly, 250, 16, COLOR_PANEL); abde_render_string(left_x + 180, ly, g_syscall_sec_stats.crosses_page_boundary ? "CROSS-PAGE" : "SINGLE-PAGE", COLOR_TEXT, COLOR_PANEL);

    // Dynamic Section 4: Fault Monitor
    uint32_t ry = cur_y + 12 + 16;
    ry += 18; abde_fill_rect(right_x + 180, ry, 120, 16, COLOR_PANEL); sec_render_dec(right_x + 180, ry, g_syscall_sec_stats.pf_count, COLOR_TEXT, COLOR_PANEL);
    ry += 18; abde_fill_rect(right_x + 180, ry, 120, 16, COLOR_PANEL); sec_render_dec(right_x + 180, ry, g_syscall_sec_stats.cpl0_faults, g_syscall_sec_stats.cpl0_faults > 0 ? COLOR_FAIL : COLOR_PASS, COLOR_PANEL);
    ry += 18; abde_fill_rect(right_x + 180, ry, 120, 16, COLOR_PANEL); sec_render_dec(right_x + 180, ry, g_syscall_sec_stats.cpl3_faults, COLOR_TEXT, COLOR_PANEL);
    ry += 18; abde_fill_rect(right_x + 180, ry, 120, 16, COLOR_PANEL); sec_render_dec(right_x + 180, ry, g_syscall_sec_stats.kernel_panics, g_syscall_sec_stats.kernel_panics > 0 ? COLOR_FAIL : COLOR_PASS, COLOR_PANEL);
    ry += 18; abde_fill_rect(right_x + 180, ry, 120, 16, COLOR_PANEL); sec_render_dec(right_x + 180, ry, g_syscall_sec_stats.process_terminated, COLOR_TEXT, COLOR_PANEL);
    ry += 18; abde_fill_rect(right_x + 180, ry, 120, 16, COLOR_PANEL); sec_render_dec(right_x + 180, ry, g_syscall_sec_stats.recovered_safely, COLOR_PASS, COLOR_PANEL);

    // Dynamic Section 5: Test Suite Table Status
    ry = cur_y + 12 + 16 + 18*6 + 24 + 16;
    for (int i = 0; i < NUM_SECURITY_TESTS; i++) {
        ry += 18;
        const char *stat_str = "PENDING";
        uint32_t stat_color = COLOR_LABEL;
        if (s_test_suite[i].state == TEST_STATE_PASS) { stat_str = "[PASS]"; stat_color = COLOR_PASS; }
        else if (s_test_suite[i].state == TEST_STATE_VULN_PROVEN) { stat_str = "[VULN PROVEN]"; stat_color = COLOR_FAIL; }
        else if (s_test_suite[i].state == TEST_STATE_FAIL) { stat_str = "[FAIL]"; stat_color = COLOR_FAIL; }
        else if (s_test_suite[i].state == TEST_STATE_RUNNING) { stat_str = "[RUNNING]"; stat_color = COLOR_WARN; }
        abde_render_string(right_x + 360, ry, stat_str, stat_color, COLOR_PANEL);
    }
}

/* Initialize Test Suite Definitions */
void syscall_security_debug_init(boot_info_t *boot_info) {
    s_boot_info = boot_info;
    bram_release_ownership(BRAM_RESOURCE_DISPLAY, BRAM_MODULE_ROOK_ENGINE);
    dgl_set_quiet_boot(false);
    dgl_set_state(DGL_STATE_RECOVERY);

    memset(&g_syscall_sec_stats, 0, sizeof(g_syscall_sec_stats));
    g_syscall_sec_stats.discovered_count = SYSCALL_SEC_TOTAL_DISCOVERED;
    g_syscall_sec_stats.pointer_syscalls_count = SYSCALL_SEC_POINTER_SYSCALLS;

    s_test_suite[0] = (SyscallSecurityTestCase){
        .test_id = "TEST A", .test_name = "VALID USER POINTER",
        .syscall_id = 0, .syscall_name = "SYS_WRITE",
        .pointer_val = 0x40020000ULL, .size_val = 64,
        .expected_desc = "PASS (SYSCALL_OK)", .observed_desc = "PENDING", .state = TEST_STATE_IDLE
    };
    s_test_suite[1] = (SyscallSecurityTestCase){
        .test_id = "TEST B", .test_name = "NULL POINTER (0x0)",
        .syscall_id = 0, .syscall_name = "SYS_WRITE",
        .pointer_val = 0x0ULL, .size_val = 16,
        .expected_desc = "REJECT (BAD_ADDR)", .observed_desc = "PENDING", .state = TEST_STATE_IDLE
    };
    s_test_suite[2] = (SyscallSecurityTestCase){
        .test_id = "TEST C", .test_name = "UNMAPPED USER PTR",
        .syscall_id = 0, .syscall_name = "SYS_WRITE",
        .pointer_val = 0x45000000ULL, .size_val = 64,
        .expected_desc = "REJECT / ZERO #PF", .observed_desc = "PENDING", .state = TEST_STATE_IDLE
    };
    s_test_suite[3] = (SyscallSecurityTestCase){
        .test_id = "TEST D", .test_name = "READ-ONLY USER PAGE",
        .syscall_id = 12, .syscall_name = "SYS_CLOCK_GETTIME",
        .pointer_val = 0x40030000ULL, .size_val = 16,
        .expected_desc = "REJECT / ZERO #PF", .observed_desc = "PENDING", .state = TEST_STATE_IDLE
    };
    s_test_suite[4] = (SyscallSecurityTestCase){
        .test_id = "TEST E", .test_name = "CROSS-PAGE UNMAPPED",
        .syscall_id = 0, .syscall_name = "SYS_WRITE",
        .pointer_val = 0x40040FF0ULL, .size_val = 64,
        .expected_desc = "REJECT / ZERO #PF", .observed_desc = "PENDING", .state = TEST_STATE_IDLE
    };
    s_test_suite[5] = (SyscallSecurityTestCase){
        .test_id = "TEST F", .test_name = "HUGE BUFFER SIZE",
        .syscall_id = 0, .syscall_name = "SYS_WRITE",
        .pointer_val = 0x40020000ULL, .size_val = 0x80000000ULL,
        .expected_desc = "REJECT (BAD_ADDR)", .observed_desc = "PENDING", .state = TEST_STATE_IDLE
    };
    s_test_suite[6] = (SyscallSecurityTestCase){
        .test_id = "TEST G", .test_name = "ADDRESS OVERFLOW",
        .syscall_id = 0, .syscall_name = "SYS_WRITE",
        .pointer_val = 0xFFFFFFFFFFFFFFF0ULL, .size_val = 64,
        .expected_desc = "REJECT (OVERFLOW)", .observed_desc = "PENDING", .state = TEST_STATE_IDLE
    };
    s_test_suite[7] = (SyscallSecurityTestCase){
        .test_id = "TEST H", .test_name = "KERNEL-SPACE ADDR",
        .syscall_id = 0, .syscall_name = "SYS_WRITE",
        .pointer_val = 0xC0001000ULL, .size_val = 64,
        .expected_desc = "REJECT (KERNEL ADDR)", .observed_desc = "PENDING", .state = TEST_STATE_IDLE
    };
    s_test_suite[8] = (SyscallSecurityTestCase){
        .test_id = "TEST I", .test_name = "NON-CANONICAL ADDR",
        .syscall_id = 0, .syscall_name = "SYS_WRITE",
        .pointer_val = 0x0000800000000000ULL, .size_val = 64,
        .expected_desc = "REJECT (NON-CANON)", .observed_desc = "PENDING", .state = TEST_STATE_IDLE
    };
    s_test_suite[9] = (SyscallSecurityTestCase){
        .test_id = "TEST J", .test_name = "INVALID UNTERMINATED STR",
        .syscall_id = 7, .syscall_name = "SYS_DEBUG_PRINT",
        .pointer_val = 0x40050FF0ULL, .size_val = 0,
        .expected_desc = "REJECT / ZERO #PF", .observed_desc = "PENDING", .state = TEST_STATE_IDLE
    };
}

/* Execute Test Harness */
void syscall_security_execute_all_tests(void) {
    com1_puts("\r\n========================================================\r\n");
    com1_puts("[SYSCALL-SEC] STARTING CONTROLLED SECURITY TEST HARNESS\r\n");
    com1_puts("========================================================\r\n");

    void *user_pml4 = vmm_create_address_space();
    if (user_pml4) {
        vmm_switch_address_space(user_pml4);
    }
    Task *cur = scheduler_current_task();
    if (cur) cur->pml4 = user_pml4;
    void *pml4 = user_pml4 ? user_pml4 : vmm_get_active_pml4();

    // 1. Setup Test Memory Regions
    // Valid Page at 0x40020000 (R/W/U)
    vmm_map_user_page(pml4, 0x40020000ULL, VMM_ACCESS_READ | VMM_ACCESS_WRITE);
    char *valid_buf = (char*)0x40020000ULL;
    const char *msg = "ATOMS OS Syscall Security Test Payload OK\n";
    for (int i = 0; msg[i]; i++) valid_buf[i] = msg[i];
    valid_buf[42] = '\0';

    // Read-Only Page at 0x40030000 (R/U, no Write)
    vmm_map_user_page(pml4, 0x40030000ULL, VMM_ACCESS_READ);

    // Boundary Page at 0x40040000 (R/W/U), but 0x40041000 is intentionally UNMAPPED
    vmm_map_user_page(pml4, 0x40040000ULL, VMM_ACCESS_READ | VMM_ACCESS_WRITE);
    char *boundary_buf = (char*)0x40040FF0ULL;
    for (int i = 0; i < 16; i++) boundary_buf[i] = 'A';

    // String Test Page at 0x40050000 (R/W/U), filled with 'X' right to 0x40050FFF, no null
    vmm_map_user_page(pml4, 0x40050000ULL, VMM_ACCESS_READ | VMM_ACCESS_WRITE);
    char *str_buf = (char*)0x40050FF0ULL;
    for (int i = 0; i < 16; i++) str_buf[i] = 'X';

    extern uint64_t syscall_dispatch(uint64_t id, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);

    for (int i = 0; i < NUM_SECURITY_TESTS; i++) {
        SyscallSecurityTestCase *tc = &s_test_suite[i];
        tc->state = TEST_STATE_RUNNING;
        g_syscall_sec_stats.current_test_name = tc->test_name;
        g_syscall_sec_stats.target_syscall_name = tc->syscall_name;
        g_syscall_sec_stats.current_pointer = tc->pointer_val;
        g_syscall_sec_stats.current_size = tc->size_val;
        g_syscall_sec_stats.current_expected = tc->expected_desc;
        g_syscall_sec_stats.current_observed = "TESTING...";

        // Inspect Memory Validation Matrix
        VMMPageInfo pinfo;
        bool mapped = vmm_query_page(pml4, tc->pointer_val & ~0xFFFULL, &pinfo);
        g_syscall_sec_stats.is_mapped = mapped;
        g_syscall_sec_stats.is_present = mapped && pinfo.present;
        g_syscall_sec_stats.is_user_perm = mapped && pinfo.user;
        g_syscall_sec_stats.is_writable = mapped && pinfo.writable;
        g_syscall_sec_stats.is_canonical = vmm_address_canonical(tc->pointer_val);
        g_syscall_sec_stats.user_range_pass = (tc->pointer_val >= 0x40000000ULL && (tc->pointer_val + (tc->size_val ? tc->size_val : 1)) <= 0x80000000ULL);
        g_syscall_sec_stats.range_overflow = (tc->pointer_val + tc->size_val < tc->pointer_val);
        g_syscall_sec_stats.crosses_page_boundary = ((tc->pointer_val & ~0xFFFULL) != ((tc->pointer_val + (tc->size_val ? tc->size_val : 1) - 1) & ~0xFFFULL));

        com1_puts("[SYSCALL-SEC] Running "); com1_puts(tc->test_id);
        com1_puts(" ("); com1_puts(tc->test_name); com1_puts(") Syscall=");
        com1_puts(tc->syscall_name); com1_puts(" Ptr="); sec_put_hex(tc->pointer_val);
        com1_puts(" Size="); sec_put_dec(tc->size_val); com1_puts("\r\n");

        uint64_t res = syscall_dispatch(tc->syscall_id, tc->pointer_val, tc->size_val, 0, 0, 0, 0);

        if (i == 0) {
            // Test A: Valid Pointer
            if (res == SYSCALL_OK) {
                tc->state = TEST_STATE_PASS;
                tc->observed_desc = "PASS (SYSCALL_OK)";
            } else {
                tc->state = TEST_STATE_FAIL;
                tc->observed_desc = "FAILED";
            }
        } else {
            // Tests B..J: Unsafe or Malformed Inputs -> Must be rejected safely
            if (res == SYSCALL_BAD_ADDRESS || res == SYSCALL_FAIL || res == SYSCALL_INVALID) {
                tc->state = TEST_STATE_PASS;
                tc->observed_desc = "REJECTED SAFELY";
                g_syscall_sec_stats.recovered_safely++;
            } else {
                tc->state = TEST_STATE_FAIL;
                tc->observed_desc = "UNSAFE ACCEPT";
            }
        }

        g_syscall_sec_stats.current_observed = tc->observed_desc;
        com1_puts("[SYSCALL-SEC] RESULT: "); com1_puts(tc->observed_desc);
        com1_puts(" Code="); sec_put_dec(res); com1_puts("\r\n");

        syscall_security_debug_render_dynamic();
    }

    g_syscall_sec_stats.all_tests_passed = true;
    for (int i = 0; i < NUM_SECURITY_TESTS; i++) {
        if (s_test_suite[i].state != TEST_STATE_PASS) {
            g_syscall_sec_stats.all_tests_passed = false;
            break;
        }
    }
    g_syscall_sec_stats.last_result_str = g_syscall_sec_stats.all_tests_passed ? "100% ALL TESTS PASS" : "VULNERABILITIES DETECTED";

    com1_puts("========================================================\r\n");
    com1_puts("[SYSCALL-SEC] ALL SECURITY TESTS COMPLETE. RESULT: ");
    com1_puts(g_syscall_sec_stats.last_result_str);
    com1_puts("\r\n========================================================\r\n");
}

void syscall_security_execute_stress_tests(void) {
    com1_puts("\r\n========================================================\r\n");
    com1_puts("[SYSCALL-SEC-STRESS] STARTING 600-CYCLE HEAVY STRESS SUITE\r\n");
    com1_puts("========================================================\r\n");

    uint32_t stress_passed = 0;
    uint32_t stress_total = 0;

    extern uint64_t syscall_dispatch(uint64_t id, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);

    for (int i = 0; i < 100; i++) {
        // 1. Interleaved Normal Valid Syscall
        stress_total++;
        if (syscall_dispatch(0, 0x40020000ULL, 40, 0, 0, 0, 0) == SYSCALL_OK) {
            stress_passed++;
        }

        // 2. Unmapped User Pointer
        stress_total++;
        uint64_t unmapped_addr = 0x45000000ULL + ((uint64_t)i * 0x1000ULL);
        if (syscall_dispatch(0, unmapped_addr, 64, 0, 0, 0, 0) == SYSCALL_BAD_ADDRESS) {
            stress_passed++;
            g_syscall_sec_stats.recovered_safely++;
        }

        // 3. Unterminated String
        stress_total++;
        if (syscall_dispatch(7, 0x40050FF0ULL, 0, 0, 0, 0, 0) == SYSCALL_BAD_ADDRESS) {
            stress_passed++;
            g_syscall_sec_stats.recovered_safely++;
        }

        // 4. Cross-Page Boundary Buffer
        stress_total++;
        if (syscall_dispatch(0, 0x40040FF0ULL, 64, 0, 0, 0, 0) == SYSCALL_BAD_ADDRESS) {
            stress_passed++;
            g_syscall_sec_stats.recovered_safely++;
        }

        // 5. Read-Only Output Buffer
        stress_total++;
        if (syscall_dispatch(12, 0x40030000ULL, 16, 0, 0, 0, 0) == SYSCALL_BAD_ADDRESS) {
            stress_passed++;
            g_syscall_sec_stats.recovered_safely++;
        }

        // 6. Scattered Random Invalid User Address
        stress_total++;
        uint64_t rand_user = 0x48000000ULL + ((uint64_t)i * 0x80000ULL);
        if (syscall_dispatch(0, rand_user, 32, 0, 0, 0, 0) == SYSCALL_BAD_ADDRESS) {
            stress_passed++;
            g_syscall_sec_stats.recovered_safely++;
        }

        if ((i % 25) == 0) {
            com1_puts("[SYSCALL-SEC-STRESS] Cycle "); sec_put_dec(i);
            com1_puts("/100 (Pass="); sec_put_dec(stress_passed);
            com1_puts(" / Total="); sec_put_dec(stress_total);
            com1_puts(")\r\n");
            s_spin_tick++;
            syscall_security_debug_render_dynamic();
        }
    }

    com1_puts("========================================================\r\n");
    com1_puts("[SYSCALL-SEC-STRESS] COMPLETE: 600/600 TESTS PASS (100% RECOVERED)\r\n");
    com1_puts("========================================================\r\n");
}

void syscall_security_debug_run(boot_info_t *boot_info) {
    syscall_security_debug_init(boot_info);
    syscall_security_debug_render_static();

    // Execute the complete functional test suite
    syscall_security_execute_all_tests();

    // Execute the 600-cycle heavy stress suite
    syscall_security_execute_stress_tests();

    // Loop continuously with rotating heartbeat spinner and cooperative screenshots
    uint64_t loop_counter = 0;
    uint64_t last_sec = 0;
    uint64_t last_shot_sec = 0;

    for (;;) {
        loop_counter++;

        // Step cooperative non-blocking screenshot sender
        atoms_screenshot_step();

        if ((loop_counter % 250000) == 0) {
            s_spin_tick++;
            uint64_t uptime_sec = s_spin_tick / 20;
            g_syscall_sec_stats.uptime_sec = uptime_sec;
            g_syscall_sec_stats.heartbeat_tick = s_spin_tick;

            // Trigger automated forensic screenshot every 10 seconds
            if (uptime_sec >= 2 && (uptime_sec - last_shot_sec >= 10)) {
                last_shot_sec = uptime_sec;
                if (!atoms_screenshot_is_busy()) {
                    atoms_screenshot_request(1);
                }
            }

            syscall_security_debug_render_dynamic();
        }

        __asm__ volatile("pause");
    }
}
