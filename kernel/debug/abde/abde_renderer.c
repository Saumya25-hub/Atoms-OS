#include "abde.h"
#include "abde_font.h"
#include "kernel/core/scheduler/include/scheduler.h"

extern abde_engine_t g_abde;
static uint32_t g_heartbeat_index = 0;
static const char g_heartbeat_chars[4] = {'|', '/', '-', '\\'};
static bool g_bg_initialized = false;

/* Fill Rectangle on Framebuffer */
void abde_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!g_abde.framebuffer) return;
    uint32_t pitch = g_abde.pitch;
    uint32_t max_w = g_abde.width;
    uint32_t max_h = g_abde.height;

    for (uint32_t r = y; r < y + h && r < max_h; r++) {
        uint32_t *row = (uint32_t*)(uintptr_t)(g_abde.framebuffer + r * pitch);
        for (uint32_t c = x; c < x + w && c < max_w; c++) {
            row[c] = color;
        }
    }
}

/* Render Single Character using 8x16 Bitmap Font */
void abde_render_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color) {
    if (!g_abde.framebuffer) return;
    if (c < 32 || c > 126) c = '?';

    uint32_t glyph_idx = c - 32;
    const uint8_t *glyph = abde_font_8x16[glyph_idx];
    uint32_t pitch = g_abde.pitch;

    for (uint32_t r = 0; r < ABDE_FONT_HEIGHT; r++) {
        uint32_t py = y + r;
        if (py >= g_abde.height) break;

        uint32_t *row = (uint32_t*)(uintptr_t)(g_abde.framebuffer + py * pitch);
        uint8_t bits = glyph[r];

        for (uint32_t col = 0; col < ABDE_FONT_WIDTH; col++) {
            uint32_t px = x + col;
            if (px >= g_abde.width) break;

            if (bits & (1 << (7 - col))) {
                row[px] = fg_color;
            } else if (bg_color != 0xFFFFFFFF) {
                row[px] = bg_color;
            }
        }
    }
}

/* Render Null-Terminated String */
void abde_render_string(uint32_t x, uint32_t y, const char *str, uint32_t fg_color, uint32_t bg_color) {
    if (!str) return;
    uint32_t cur_x = x;
    while (*str) {
        abde_render_char(cur_x, y, *str, fg_color, bg_color);
        cur_x += ABDE_FONT_WIDTH;
        str++;
    }
}

/* Render Null-Terminated String with Fixed Width Padding */
void abde_render_string_padded(uint32_t x, uint32_t y, const char *str, uint32_t max_chars, uint32_t fg_color, uint32_t bg_color) {
    if (!str) return;
    uint32_t cur_x = x;
    uint32_t count = 0;
    while (*str && count < max_chars) {
        abde_render_char(cur_x, y, *str, fg_color, bg_color);
        cur_x += ABDE_FONT_WIDTH;
        str++;
        count++;
    }
    while (count < max_chars) {
        abde_render_char(cur_x, y, ' ', fg_color, bg_color);
        cur_x += ABDE_FONT_WIDTH;
        count++;
    }
}

/* Render Decimal Integer Helper */
static void abde_render_dec(uint32_t x, uint32_t y, uint32_t val, uint32_t fg_color, uint32_t bg_color) {
    char buf[12];
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
    } else {
        int idx = 10;
        buf[11] = '\0';
        while (val > 0 && idx >= 0) {
            buf[idx--] = '0' + (val % 10);
            val /= 10;
        }
        abde_render_string(x, y, &buf[idx + 1], fg_color, bg_color);
        return;
    }
    abde_render_string(x, y, buf, fg_color, bg_color);
}

/* Render Hexadecimal Integer Helper */
static void abde_render_hex(uint32_t x, uint32_t y, uint64_t val, uint32_t fg_color, uint32_t bg_color) {
    char buf[19] = "0x0000000000000000";
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 15; i >= 0; i--) {
        buf[2 + i] = hex_chars[(val >> ((15 - i) * 4)) & 0xF];
    }
    buf[18] = '\0';
    abde_render_string(x, y, buf, fg_color, bg_color);
}

/* Render ABDE V2.5 Real-Time Forensic Dashboard Screen */
void diag_render(void) {
    if (!g_abde.framebuffer) return;

    uint32_t bg_color     = 0x000F172A; // Dark Slate Blue Background
    uint32_t panel_bg     = 0x001E293B; // Deep Slate Panel Fill
    uint32_t text_color   = 0x00F8FAFC; // Bright White Text
    uint32_t border_color = 0x00334155; // Slate Border
    uint32_t label_color  = 0x0094A3B8; // Slate Gray Label
    uint32_t pass_color   = 0x0022C55E; // Bright Neon Green PASS
    uint32_t run_color    = 0x00EAB308; // Amber Yellow RUNNING
    uint32_t fail_color   = 0x00EF4444; // Crimson Red FAIL
    uint32_t info_color   = 0x0038BDF8; // Electric Cyan INFO

    uint32_t start_x = 30;
    uint32_t start_y = 20;
    uint32_t cur_y   = start_y + 76;
    uint32_t left_x  = start_x;
    uint32_t right_x = start_x + 360;

    // Clear main background and panels once on initialization
    if (!g_bg_initialized) {
        abde_fill_rect(0, 0, g_abde.width, g_abde.height, bg_color);
        abde_fill_rect(left_x, cur_y, 340, 260, panel_bg);
        abde_fill_rect(right_x, cur_y, 400, 260, panel_bg);
        g_bg_initialized = true;
    }

    // Header Banner
    abde_render_string(start_x, start_y,      "==========================================================================================", text_color, bg_color);
    abde_render_string(start_x, start_y + 18, " ATOMS OS REAL-TIME FORENSIC DASHBOARD V2.5", info_color, bg_color);

    // Heartbeat Spinner
    char hb_buf[16] = "Heartbeat:  ";
    hb_buf[11] = g_heartbeat_chars[g_heartbeat_index % 4];
    g_heartbeat_index++;
    abde_render_string(start_x + 560, start_y + 18, hb_buf, 0x00F59E0B, bg_color);

    abde_render_string(start_x, start_y + 36, " Subsystem Validation Stack & Live Multi-Core Bring-Up Tracker", label_color, bg_color);
    abde_render_string(start_x, start_y + 54, "==========================================================================================", text_color, bg_color);

    // =========================================================================
    // SECTION 1: SUBSYSTEM CERTIFICATION BOARD (LEFT PANEL)
    // =========================================================================
    abde_render_string(left_x + 10, cur_y + 10, "[ SUBSYSTEM STATUS BOARD ]", info_color, panel_bg);

    uint32_t board_y = cur_y + 34;
    for (uint32_t i = 0; i < g_abde.module_count && i < 10; i++) {
        diag_module_t *mod = &g_abde.modules[i];

        abde_render_string_padded(left_x + 15, board_y, mod->name, 12, text_color, panel_bg);

        // Leader dots
        for (uint32_t dx = left_x + 120; dx < left_x + 220; dx += 10) {
            abde_render_string(dx, board_y, ".", border_color, panel_bg);
        }

        // Status Tag
        uint32_t status_x = left_x + 230;
        switch (mod->status) {
            case DIAG_STATUS_PASS:
                abde_render_string_padded(status_x, board_y, "PASS  [OK]", 10, pass_color, panel_bg);
                break;
            case DIAG_STATUS_RUNNING:
                abde_render_string_padded(status_x, board_y, "RUNNING   ", 10, run_color, panel_bg);
                break;
            case DIAG_STATUS_FAIL:
                abde_render_string_padded(status_x, board_y, "FAIL!     ", 10, fail_color, panel_bg);
                break;
            case DIAG_STATUS_WAIT:
            default:
                abde_render_string_padded(status_x, board_y, "WAIT      ", 10, 0x0064748B, panel_bg);
                break;
        }

        board_y += 20;
    }

    // =========================================================================
    // SECTION 3: DYNAMIC VMM / PMM / PIC / IDT TELEMETRY PANEL (RIGHT PANEL)
    // Clear right panel background to prevent text overlap between module transitions
    // =========================================================================
    abde_fill_rect(right_x, cur_y, 400, 260, panel_bg);

    // BOE V5.0 FORENSIC HARDENING: Only render heap panel if data is provably valid.
    // On H81 bare metal, BSS may contain garbage before heap_init() runs.
    // heap_active must be exactly 1 (true), and heap_base must equal the known constant.
    bool heap_data_valid = (g_abde.heap_active == true)
                        && (g_abde.heap_base == 0xC0000000ULL)
                        && (g_abde.heap_size_kb <= 1048576);  // <= 1GB sanity limit

    if (g_abde.sched_active || abde_streq(g_abde.current_module, "SCHED")) {
        abde_render_string(right_x + 10, cur_y + 8, "[ SCHEDULER FORENSIC PANEL ]", info_color, panel_bg);

        uint32_t sy = cur_y + 26;
        abde_render_string(right_x + 15, sy, "Scheduler Init :", label_color, panel_bg);
        abde_render_string_padded(right_x + 180, sy, "PASS [OK]", 10, pass_color, panel_bg);
        sy += 16;

        bool sched_on = scheduler_is_running();
        abde_render_string(right_x + 15, sy, "Scheduler State:", label_color, panel_bg);
        abde_render_string_padded(right_x + 180, sy, sched_on ? "ACTIVE" : "STARTING", 10, pass_color, panel_bg);
        sy += 16;

        Task *cur = sched_on ? scheduler_current_task() : NULL;
        uint64_t ticks = scheduler_get_tick_count();
        uint64_t switches = scheduler_get_context_switch_count();
        uint32_t ready_cnt = scheduler_get_ready_count();
        uint32_t sleep_cnt = scheduler_get_sleeping_count();
        uint32_t block_cnt = scheduler_get_blocked_count();
        uint64_t last_old = scheduler_get_last_switch_from();
        uint64_t last_new = scheduler_get_last_switch_to();
        const char *reason_str = scheduler_get_last_reason_str();

        bool cur_valid = (cur != NULL && (uint64_t)cur >= 0x100000ULL && (uint64_t)cur < 0xFFFFFFFF00000000ULL);
        const char *cur_name = "ABDE_Telemetry";
        if (cur_valid && cur->name && (uint64_t)cur->name >= 0x100000ULL && (uint64_t)cur->name < 0xFFFFFFFF00000000ULL) {
            cur_name = cur->name;
        }

        abde_render_string(right_x + 15, sy, "Current Task   :", label_color, panel_bg);
        abde_render_string_padded(right_x + 180, sy, cur_name, 16, pass_color, panel_bg);
        sy += 16;

        uint32_t cur_id = cur_valid ? (uint32_t)cur->id : 0;
        uint32_t cur_prio = cur_valid ? (uint32_t)cur->effective_priority : 24;
        abde_render_string(right_x + 15, sy, "Task ID / Prio :", label_color, panel_bg);
        abde_render_dec(right_x + 180, sy, cur_id, text_color, panel_bg);
        abde_render_string(right_x + 220, sy, "/ p", label_color, panel_bg);
        abde_render_dec(right_x + 245, sy, cur_prio, text_color, panel_bg);
        sy += 16;

        abde_render_string(right_x + 15, sy, "Ready/Slp/Blk  :", label_color, panel_bg);
        abde_render_dec(right_x + 180, sy, ready_cnt, pass_color, panel_bg);
        abde_render_string(right_x + 205, sy, "/", label_color, panel_bg);
        abde_render_dec(right_x + 215, sy, sleep_cnt, info_color, panel_bg);
        abde_render_string(right_x + 240, sy, "/", label_color, panel_bg);
        abde_render_dec(right_x + 250, sy, block_cnt, run_color, panel_bg);
        sy += 16;

        abde_render_string(right_x + 15, sy, "Tick / Switches:", label_color, panel_bg);
        abde_render_dec(right_x + 180, sy, (uint32_t)ticks, text_color, panel_bg);
        abde_render_string(right_x + 230, sy, "/", label_color, panel_bg);
        abde_render_dec(right_x + 245, sy, (uint32_t)switches, info_color, panel_bg);
        sy += 16;

        abde_render_string(right_x + 15, sy, "Last Switch    :", label_color, panel_bg);
        abde_render_dec(right_x + 180, sy, (uint32_t)last_old, text_color, panel_bg);
        abde_render_string(right_x + 205, sy, "->", label_color, panel_bg);
        abde_render_dec(right_x + 225, sy, (uint32_t)last_new, pass_color, panel_bg);
        sy += 16;

        abde_render_string(right_x + 15, sy, "Switch Reason  :", label_color, panel_bg);
        abde_render_string_padded(right_x + 180, sy, reason_str ? reason_str : "NONE", 12, info_color, panel_bg);
        sy += 22;

        // Section 3B: Thread Execution Sub-Panel
        abde_render_string(right_x + 10, sy, "[ THREAD EXECUTION PANEL ]", info_color, panel_bg);
        sy += 18;

        if (sched_on) {
            for (uint32_t ti = 0; ti < 4; ti++) {
                Task *t = scheduler_get_system_thread(ti);
                bool t_valid = (t != NULL && (uint64_t)t >= 0x100000ULL && (uint64_t)t < 0xFFFFFFFF00000000ULL);
                if (t_valid) {
                    const char *st_str = "RUN";
                    if (t->state == TASK_SLEEPING) st_str = "SLP";
                    else if (t->state == TASK_READY) st_str = "RDY";
                    else if (t->state == TASK_BLOCKED) st_str = "BLK";
                    else if (t->state == TASK_WAITING) st_str = "WAT";
                    
                    uint32_t col = (t == cur) ? pass_color : text_color;
                    const char *t_name = "Task";
                    if (t->name && (uint64_t)t->name >= 0x100000ULL && (uint64_t)t->name < 0xFFFFFFFF00000000ULL) {
                        t_name = t->name;
                    }
                    abde_render_string_padded(right_x + 15, sy, t_name, 14, col, panel_bg);
                    abde_render_string(right_x + 130, sy, ": T", label_color, panel_bg);
                    abde_render_dec(right_x + 148, sy, (uint32_t)t->id, text_color, panel_bg);
                    abde_render_string(right_x + 162, sy, "[", label_color, panel_bg);
                    abde_render_string(right_x + 170, sy, st_str, col, panel_bg);
                    abde_render_string(right_x + 195, sy, "] Ticks:", label_color, panel_bg);
                    abde_render_dec(right_x + 252, sy, (uint32_t)t->total_run_ticks, info_color, panel_bg);
                    sy += 16;
                }
            }
        } else {
            abde_render_string(right_x + 15, sy, "System threads initializing...", label_color, panel_bg);
        }
    } else if (heap_data_valid) {
        abde_render_string(right_x + 10, cur_y + 8, "[ HEAP LIVE TELEMETRY PANEL ]", info_color, panel_bg);

        // Force null-terminate status string to prevent string overrun on garbage BSS
        g_abde.heap_status_str[15] = '\0';

        uint32_t heap_y = cur_y + 26;
        abde_render_string(right_x + 15, heap_y, "Heap Base       :", label_color, panel_bg);
        abde_render_hex(right_x + 180, heap_y, g_abde.heap_base, pass_color, panel_bg);
        heap_y += 18;

        abde_render_string(right_x + 15, heap_y, "Heap Size       :", label_color, panel_bg);
        abde_render_dec(right_x + 180, heap_y, (uint32_t)g_abde.heap_size_kb, text_color, panel_bg);
        abde_render_string_padded(right_x + 250, heap_y, "KB", 4, label_color, panel_bg);
        heap_y += 18;

        abde_render_string(right_x + 15, heap_y, "Used Memory     :", label_color, panel_bg);
        abde_render_dec(right_x + 180, heap_y, (uint32_t)g_abde.heap_used_kb, info_color, panel_bg);
        abde_render_string_padded(right_x + 250, heap_y, "KB", 4, label_color, panel_bg);
        heap_y += 18;

        abde_render_string(right_x + 15, heap_y, "Free Memory     :", label_color, panel_bg);
        abde_render_dec(right_x + 180, heap_y, (uint32_t)g_abde.heap_free_kb, pass_color, panel_bg);
        abde_render_string_padded(right_x + 250, heap_y, "KB", 4, label_color, panel_bg);
        heap_y += 18;

        abde_render_string(right_x + 15, heap_y, "Allocations     :", label_color, panel_bg);
        abde_render_dec(right_x + 180, heap_y, (uint32_t)g_abde.heap_alloc_count, pass_color, panel_bg);
        heap_y += 18;

        abde_render_string(right_x + 15, heap_y, "Frees           :", label_color, panel_bg);
        abde_render_dec(right_x + 180, heap_y, (uint32_t)g_abde.heap_free_count, info_color, panel_bg);
        heap_y += 18;

        abde_render_string(right_x + 15, heap_y, "Corruptions     :", label_color, panel_bg);
        abde_render_dec(right_x + 180, heap_y, (uint32_t)g_abde.heap_corruption_count, g_abde.heap_corruption_count > 0 ? fail_color : pass_color, panel_bg);
        heap_y += 18;

        abde_render_string(right_x + 15, heap_y, "Last Alloc Addr :", label_color, panel_bg);
        abde_render_hex(right_x + 180, heap_y, g_abde.heap_last_alloc, info_color, panel_bg);
        heap_y += 18;

        abde_render_string(right_x + 15, heap_y, "Last Caller RIP :", label_color, panel_bg);
        abde_render_hex(right_x + 180, heap_y, g_abde.heap_last_caller_rip, text_color, panel_bg);
        heap_y += 18;

        abde_render_string(right_x + 15, heap_y, "Heap Status     :", label_color, panel_bg);
        uint32_t st_color = pass_color;
        if (g_abde.heap_status_str[0] == 'R') st_color = run_color;
        else if (g_abde.heap_status_str[0] == 'F') st_color = fail_color;
        abde_render_string_padded(right_x + 180, heap_y, g_abde.heap_status_str[0] ? g_abde.heap_status_str : "WAIT", 12, st_color, panel_bg);
    } else if (g_abde.heap_active) {
        // heap_active is set but data looks corrupted — show INVALID DATA
        abde_render_string(right_x + 10, cur_y + 8, "[ HEAP LIVE TELEMETRY PANEL ]", info_color, panel_bg);
        abde_render_string(right_x + 15, cur_y + 40, "HEAP STATUS : INVALID DATA", fail_color, panel_bg);
        abde_render_string(right_x + 15, cur_y + 60, "Waiting for valid heap init...", label_color, panel_bg);
    } else if (g_abde.vmm_active) {
        abde_render_string(right_x + 10, cur_y + 8, "[ VMM LIVE TELEMETRY PANEL ]", info_color, panel_bg);

        uint32_t vmm_y = cur_y + 26;
        abde_render_string(right_x + 15, vmm_y, "CR3 Base        :", label_color, panel_bg);
        abde_render_hex(right_x + 180, vmm_y, g_abde.vmm_cr3, pass_color, panel_bg);
        vmm_y += 18;

        abde_render_string(right_x + 15, vmm_y, "PML4 Address    :", label_color, panel_bg);
        abde_render_hex(right_x + 180, vmm_y, g_abde.vmm_pml4, text_color, panel_bg);
        vmm_y += 18;

        abde_render_string(right_x + 15, vmm_y, "PDPT Address    :", label_color, panel_bg);
        abde_render_hex(right_x + 180, vmm_y, g_abde.vmm_pdpt, text_color, panel_bg);
        vmm_y += 18;

        abde_render_string(right_x + 15, vmm_y, "Identity Pages  :", label_color, panel_bg);
        abde_render_dec(right_x + 180, vmm_y, (uint32_t)g_abde.vmm_identity_pages, pass_color, panel_bg);
        vmm_y += 18;

        abde_render_string(right_x + 15, vmm_y, "Mapped Pages    :", label_color, panel_bg);
        abde_render_dec(right_x + 180, vmm_y, (uint32_t)g_abde.vmm_mapped_pages, info_color, panel_bg);
        vmm_y += 18;

        abde_render_string(right_x + 15, vmm_y, "Page Faults     :", label_color, panel_bg);
        abde_render_dec(right_x + 180, vmm_y, (uint32_t)g_abde.vmm_page_faults, g_abde.vmm_page_faults > 0 ? fail_color : pass_color, panel_bg);
        vmm_y += 18;

        abde_render_string(right_x + 15, vmm_y, "Last Mapping    :", label_color, panel_bg);
        abde_render_hex(right_x + 180, vmm_y, g_abde.vmm_last_mapping, info_color, panel_bg);
        vmm_y += 18;

        abde_render_string(right_x + 15, vmm_y, "Last Virt Addr  :", label_color, panel_bg);
        abde_render_hex(right_x + 180, vmm_y, g_abde.vmm_last_virt, info_color, panel_bg);
        vmm_y += 18;

        abde_render_string(right_x + 15, vmm_y, "Last Phys Addr  :", label_color, panel_bg);
        abde_render_hex(right_x + 180, vmm_y, g_abde.vmm_last_phys, info_color, panel_bg);
        vmm_y += 18;

        abde_render_string(right_x + 15, vmm_y, "VMM Status      :", label_color, panel_bg);
        uint32_t st_color = pass_color;
        if (g_abde.vmm_status_str[0] == 'R') st_color = run_color;
        else if (g_abde.vmm_status_str[0] == 'F') st_color = fail_color;
        abde_render_string_padded(right_x + 180, vmm_y, g_abde.vmm_status_str[0] ? g_abde.vmm_status_str : "WAIT", 12, st_color, panel_bg);
    } else if (g_abde.pmm_active) {
        abde_render_string(right_x + 10, cur_y + 10, "[ PMM LIVE TELEMETRY PANEL ]", info_color, panel_bg);

        uint32_t pmm_y = cur_y + 34;
        abde_render_string(right_x + 15, pmm_y, "Total RAM     :", label_color, panel_bg);
        abde_render_dec(right_x + 160, pmm_y, (uint32_t)g_abde.pmm_total_ram_mb, text_color, panel_bg);
        abde_render_string_padded(right_x + 250, pmm_y, "MB", 4, label_color, panel_bg);
        pmm_y += 20;

        abde_render_string(right_x + 15, pmm_y, "Usable RAM    :", label_color, panel_bg);
        abde_render_dec(right_x + 160, pmm_y, (uint32_t)g_abde.pmm_usable_ram_mb, pass_color, panel_bg);
        abde_render_string_padded(right_x + 250, pmm_y, "MB", 4, label_color, panel_bg);
        pmm_y += 20;

        abde_render_string(right_x + 15, pmm_y, "Reserved RAM  :", label_color, panel_bg);
        abde_render_dec(right_x + 160, pmm_y, (uint32_t)g_abde.pmm_reserved_ram_mb, run_color, panel_bg);
        abde_render_string_padded(right_x + 250, pmm_y, "MB", 4, label_color, panel_bg);
        pmm_y += 20;

        abde_render_string(right_x + 15, pmm_y, "Free Pages    :", label_color, panel_bg);
        abde_render_dec(right_x + 160, pmm_y, (uint32_t)g_abde.pmm_free_pages, pass_color, panel_bg);
        abde_render_string_padded(right_x + 250, pmm_y, "Pages", 6, label_color, panel_bg);
        pmm_y += 20;

        abde_render_string(right_x + 15, pmm_y, "Used Pages    :", label_color, panel_bg);
        abde_render_dec(right_x + 160, pmm_y, (uint32_t)g_abde.pmm_used_pages, info_color, panel_bg);
        abde_render_string_padded(right_x + 250, pmm_y, "Pages", 6, label_color, panel_bg);
        pmm_y += 20;

        abde_render_string(right_x + 15, pmm_y, "Last Alloc    :", label_color, panel_bg);
        abde_render_hex(right_x + 160, pmm_y, g_abde.pmm_last_alloc, info_color, panel_bg);
    } else if (g_abde.pic_remapped) {
        abde_render_string(right_x + 10, cur_y + 10, "[ PIC/APIC LIVE TELEMETRY ]", info_color, panel_bg);

        uint32_t pic_y = cur_y + 34;
        abde_render_string(right_x + 15, pic_y, "PIC Status    :", label_color, panel_bg);
        abde_render_string(right_x + 160, pic_y, "REMAPPED 0x20", pass_color, panel_bg);
        pic_y += 20;

        abde_render_string(right_x + 15, pic_y, "APIC Status   :", label_color, panel_bg);
        abde_render_string(right_x + 160, pic_y, g_abde.apic_enabled ? "ENABLED MSR" : "DISABLED", g_abde.apic_enabled ? pass_color : run_color, panel_bg);
        pic_y += 20;

        abde_render_string(right_x + 15, pic_y, "Timer IRQ0    :", label_color, panel_bg);
        abde_render_dec(right_x + 160, pic_y, (uint32_t)g_abde.timer_irq0_ticks, pass_color, panel_bg);
        abde_render_string_padded(right_x + 250, pic_y, "Ticks", 6, label_color, panel_bg);
        pic_y += 20;

        abde_render_string(right_x + 15, pic_y, "Kbd IRQ1      :", label_color, panel_bg);
        abde_render_dec(right_x + 160, pic_y, (uint32_t)g_abde.kbd_irq1_count, info_color, panel_bg);
        abde_render_string_padded(right_x + 250, pic_y, "Events", 6, label_color, panel_bg);
        pic_y += 20;

        abde_render_string(right_x + 15, pic_y, "Last IRQ      :", label_color, panel_bg);
        abde_render_string(right_x + 160, pic_y, "IRQ ", label_color, panel_bg);
        abde_render_dec(right_x + 200, pic_y, g_abde.last_irq, text_color, panel_bg);
        pic_y += 20;

        abde_render_string(right_x + 15, pic_y, "Last Vector   :", label_color, panel_bg);
        abde_render_string(right_x + 160, pic_y, "0x", label_color, panel_bg);
        abde_render_dec(right_x + 180, pic_y, g_abde.last_vector, info_color, panel_bg);
    } else if (g_abde.idt_entries > 0) {
        abde_render_string(right_x + 10, cur_y + 10, "[ IDT LIVE TELEMETRY PANEL ]", info_color, panel_bg);

        uint32_t idt_y = cur_y + 34;
        abde_render_string(right_x + 15, idt_y, "IDT Entries   :", label_color, panel_bg);
        abde_render_dec(right_x + 160, idt_y, g_abde.idt_entries, text_color, panel_bg);
        idt_y += 20;

        abde_render_string(right_x + 15, idt_y, "IDTR Status   :", label_color, panel_bg);
        abde_render_string(right_x + 160, idt_y, "LOADED 100%", pass_color, panel_bg);
        idt_y += 20;

        abde_render_string(right_x + 15, idt_y, "ISR Handlers  :", label_color, panel_bg);
        abde_render_dec(right_x + 160, idt_y, g_abde.isr_installed, info_color, panel_bg);
        abde_render_string(right_x + 200, idt_y, "Installed", label_color, panel_bg);
        idt_y += 20;

        abde_render_string(right_x + 15, idt_y, "Exceptions    :", label_color, panel_bg);
        abde_render_string(right_x + 160, idt_y, g_abde.exceptions_armed ? "ARMED 0-31" : "WAIT", g_abde.exceptions_armed ? pass_color : run_color, panel_bg);
        idt_y += 20;

        abde_render_string(right_x + 15, idt_y, "Last Exc      :", label_color, panel_bg);
        abde_render_string_padded(right_x + 160, idt_y, g_abde.last_exception[0] ? g_abde.last_exception : "NONE", 20, info_color, panel_bg);
        idt_y += 20;

        abde_render_string(right_x + 15, idt_y, "Fault Count   :", label_color, panel_bg);
        abde_render_dec(right_x + 160, idt_y, g_abde.fault_count, g_abde.fault_count > 0 ? fail_color : pass_color, panel_bg);
    } else {
        abde_render_string(right_x + 10, cur_y + 10, "[ SMP LIVE TELEMETRY PANEL ]", info_color, panel_bg);

        uint32_t smp_y = cur_y + 34;
        abde_render_string(right_x + 15, smp_y, "BSP Core ID   :", label_color, panel_bg);
        abde_render_dec(right_x + 160, smp_y, g_abde.smp_bsp_id, text_color, panel_bg);
        smp_y += 20;

        abde_render_string(right_x + 15, smp_y, "CPUs Found    :", label_color, panel_bg);
        abde_render_dec(right_x + 160, smp_y, g_abde.smp_cpu_found, info_color, panel_bg);
        abde_render_string(right_x + 180, smp_y, "Cores", label_color, panel_bg);
        smp_y += 20;

        abde_render_string(right_x + 15, smp_y, "CPUs Online   :", label_color, panel_bg);
        abde_render_dec(right_x + 160, smp_y, g_abde.smp_cpu_online, pass_color, panel_bg);
        abde_render_string(right_x + 175, smp_y, "/", label_color, panel_bg);
        abde_render_dec(right_x + 190, smp_y, g_abde.smp_cpu_found, info_color, panel_bg);
        smp_y += 20;

        abde_render_string(right_x + 15, smp_y, "Target AP     :", label_color, panel_bg);
        abde_render_string(right_x + 160, smp_y, "CPU", label_color, panel_bg);
        abde_render_dec(right_x + 190, smp_y, g_abde.smp_current_cpu, run_color, panel_bg);
        smp_y += 20;

        abde_render_string(right_x + 15, smp_y, "INIT / SIPIs  :", label_color, panel_bg);
        abde_render_dec(right_x + 160, smp_y, g_abde.smp_init_ipis, text_color, panel_bg);
        abde_render_string(right_x + 180, smp_y, "INIT /", label_color, panel_bg);
        abde_render_dec(right_x + 240, smp_y, g_abde.smp_sipis_sent, text_color, panel_bg);
        abde_render_string(right_x + 260, smp_y, "SIPI", label_color, panel_bg);
        smp_y += 20;

        abde_render_string(right_x + 15, smp_y, "AP Responses  :", label_color, panel_bg);
        abde_render_dec(right_x + 160, smp_y, g_abde.smp_ap_responses, pass_color, panel_bg);
        abde_render_string(right_x + 180, smp_y, "ACKs", label_color, panel_bg);
    }

    cur_y += 235;

    // =========================================================================
    // SECTION 2: CURRENT EXECUTION & FORENSIC FAULT PANEL
    // =========================================================================
    abde_render_string(start_x, cur_y, "------------------------------------------------------------------------------------------", border_color, bg_color);
    cur_y += 16;

    abde_render_string_padded(start_x, cur_y, "Current Module :", 18, label_color, bg_color);
    abde_render_string_padded(start_x + 160, cur_y, g_abde.current_module[0] ? g_abde.current_module : "NONE", 45, text_color, bg_color);
    cur_y += 18;

    abde_render_string_padded(start_x, cur_y, "Current Step   :", 18, label_color, bg_color);
    abde_render_string_padded(start_x + 160, cur_y, g_abde.current_step[0] ? g_abde.current_step : "NONE", 45, text_color, bg_color);
    cur_y += 18;

    abde_render_string_padded(start_x, cur_y, "Last Event     :", 18, label_color, bg_color);
    abde_render_string_padded(start_x + 160, cur_y, g_abde.last_event[0] ? g_abde.last_event : "NONE", 45, info_color, bg_color);
    cur_y += 18;

    abde_render_string_padded(start_x, cur_y, "Overall Status :", 18, label_color, bg_color);
    if (g_abde.overall_status == DIAG_STATUS_PASS) {
        abde_render_string_padded(start_x + 160, cur_y, "PASSED (ALL CERTIFIED)", 45, pass_color, bg_color);
    } else if (g_abde.overall_status == DIAG_STATUS_FAIL) {
        abde_render_string_padded(start_x + 160, cur_y, "FAILED (SAFE HALT)", 45, fail_color, bg_color);
    } else {
        abde_render_string_padded(start_x + 160, cur_y, "RUNNING", 45, run_color, bg_color);
    }
    cur_y += 18;

    abde_render_string_padded(start_x, cur_y, "Error Code     :", 18, label_color, bg_color);
    abde_render_string_padded(start_x + 160, cur_y, g_abde.error_code[0] ? g_abde.error_code : "NONE", 45, g_abde.overall_status == DIAG_STATUS_FAIL ? fail_color : text_color, bg_color);
    cur_y += 18;

    abde_render_string_padded(start_x, cur_y, "Fault Detail   :", 18, label_color, bg_color);
    abde_render_string_padded(start_x + 160, cur_y, g_abde.fault_detail[0] ? g_abde.fault_detail : "NONE", 45, g_abde.overall_status == DIAG_STATUS_FAIL ? fail_color : text_color, bg_color);
    cur_y += 24;

    // =========================================================================
    // SECTION 4: LIVE CPU HEARTBEAT GRID
    // =========================================================================
    abde_render_string(start_x, cur_y, "------------------------------------------------------------------------------------------", border_color, bg_color);
    cur_y += 16;
    abde_render_string(start_x, cur_y, "[ LIVE PER-CPU HEARTBEAT MONITOR GRID ]", info_color, bg_color);
    cur_y += 20;

    uint32_t grid_x = start_x;
    for (int i = 0; i < 4; i++) {
        char cpu_name[16] = "CPU ";
        cpu_name[3] = '0' + i;
        cpu_name[4] = '\0';

        abde_render_string(grid_x, cur_y, cpu_name, text_color, bg_color);
        if (g_abde.cpus[i].online) {
            abde_render_string(grid_x + 40, cur_y, "[ONLINE]", pass_color, bg_color);
            abde_render_string(grid_x + 110, cur_y, "<3", 0x00F59E0B, bg_color);
            abde_render_dec(grid_x + 130, cur_y, (uint32_t)g_abde.cpus[i].heartbeat, text_color, bg_color);
        } else if (g_abde.cpus[i].failed) {
            abde_render_string(grid_x + 40, cur_y, "[FAULT]", fail_color, bg_color);
        } else {
            abde_render_string(grid_x + 40, cur_y, "[OFFLINE]", 0x0064748B, bg_color);
        }
        grid_x += 180;
    }
    cur_y += 26;

    abde_render_string(start_x, cur_y, "==========================================================================================", text_color, bg_color);
}
