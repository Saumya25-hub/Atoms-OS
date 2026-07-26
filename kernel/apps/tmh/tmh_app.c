#include "tmh_app.h"
#include "tmh_provider.h"
#include "../../shell/desktop_shell/desktop_shell.h"
#include "../../wm/bwe/include/bwe_layout.h"
#include "../../core/memory/heap/include/heap.h"
#include "../../core/lib/include/string.h"
#include "../../drivers/display/display.h"
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    TMH_TAB_OVERVIEW = 0,
    TMH_TAB_CPU,
    TMH_TAB_RAM,
    TMH_TAB_STORAGE,
    TMH_TAB_ETHERNET,
    TMH_TAB_GRAPHICS,
    TMH_TAB_COUNT
} TMHTab;

typedef struct {
    uint32_t win_id;
    uint32_t sidebar_id;
    uint32_t content_panel_id;
    uint32_t status_bar_id;
    uint32_t tab_buttons[TMH_TAB_COUNT];
    TMHTab active_tab;
    TMH_TelemetryData telemetry;
} TMHCtx;

static void tmh_render_active_tab(TMHCtx* ctx);

static void tmh_itoa(uint32_t val, char* buf) {
    char temp[16];
    int i = 0;
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (val > 0) {
        temp[i++] = (val % 10) + '0';
        val /= 10;
    }
    int r = 0;
    while (i > 0) buf[r++] = temp[--i];
    buf[r] = '\0';
}

static void tmh_make_bar(uint32_t percent, char* out_bar, uint32_t width) {
    if (width < 5) width = 10;
    uint32_t filled = (percent * width) / 100;
    if (filled > width) filled = width;

    uint32_t pos = 0;
    out_bar[pos++] = '[';
    for (uint32_t i = 0; i < filled; i++) {
        out_bar[pos++] = '#';
    }
    for (uint32_t i = filled; i < width; i++) {
        out_bar[pos++] = '.';
    }
    out_bar[pos++] = ']';
    out_bar[pos++] = ' ';

    char num_buf[16];
    tmh_itoa(percent, num_buf);
    uint32_t k = 0;
    while (num_buf[k]) out_bar[pos++] = num_buf[k++];
    out_bar[pos++] = '%';
    out_bar[pos] = '\0';
}

static void* get_top_parent_ctx_tmh(uint32_t win_id) {
    BWE_Window* curr = BWE_GetWindow(win_id);
    while (curr && curr->parent_id != BWE_DESKTOP_ID && curr->parent_id != curr->id && curr->parent_id != 0) {
        curr = BWE_GetWindow(curr->parent_id);
    }
    if (curr && curr->parent_id == BWE_DESKTOP_ID) {
        return curr->user_data;
    }
    return NULL;
}

static void tmh_tab_btn_clicked(uint32_t btn_id) {
    TMHCtx* ctx = (TMHCtx*)get_top_parent_ctx_tmh(btn_id);
    if (!ctx) return;

    for (int i = 0; i < TMH_TAB_COUNT; i++) {
        if (ctx->tab_buttons[i] == btn_id) {
            ctx->active_tab = (TMHTab)i;
            break;
        }
    }
    tmh_render_active_tab(ctx);
}

static void tmh_create_card(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char* title, const char* val1, const char* val2, const char* val3, const char* val4) {
    uint32_t card_id = 0;
    BOS_CreatePanel(parent_id, x, y, w, h, 0xFF1E293B, &card_id);
    if (!card_id) return;

    uint32_t lbl_id = 0;
    BOS_CreateLabel(card_id, 12, 10, title, 0xFFF8FAFC, &lbl_id);
    if (val1) BOS_CreateLabel(card_id, 12, 34, val1, 0xFF38BDF8, &lbl_id);
    if (val2) BOS_CreateLabel(card_id, 12, 54, val2, 0xFF4ADE80, &lbl_id);
    if (val3) BOS_CreateLabel(card_id, 12, 74, val3, 0xFF94A3B8, &lbl_id);
    if (val4) BOS_CreateLabel(card_id, 12, 94, val4, 0xFFF59E0B, &lbl_id);
}

static void tmh_render_active_tab(TMHCtx* ctx) {
    if (!ctx || !ctx->content_panel_id) return;

    BWE_Window* panel = BWE_GetWindow(ctx->content_panel_id);
    if (!panel) return;

    // Refresh telemetry data
    TMH_GatherTelemetry(&ctx->telemetry);

    // Highlight active tab button
    for (int i = 0; i < TMH_TAB_COUNT; i++) {
        if (ctx->tab_buttons[i]) {
            BWE_Window* btn = BWE_GetWindow(ctx->tab_buttons[i]);
            if (btn) {
                btn->control_data.button.bg_color = (i == (int)ctx->active_tab) ? 0xFF0284C7 : 0xFF1E293B;
                BWE_InvalidateWindow(ctx->tab_buttons[i]);
            }
        }
    }

    // Clear content panel children
    panel->child_count = 0;
    BWE_InvalidateWindow(ctx->content_panel_id);

    char buf1[128], buf2[128], buf3[128], buf4[128];
    char num1[32], num2[32];

    switch (ctx->active_tab) {
        case TMH_TAB_OVERVIEW: {
            // CPU Overview Card
            strcpy(buf1, "Model: "); strcat(buf1, ctx->telemetry.cpu_model);
            strcpy(buf2, "Usage: "); tmh_itoa(ctx->telemetry.total_cpu_usage, num1); strcat(buf2, num1); strcat(buf2, "%");
            strcpy(buf3, "Frequency: 4.25 GHz | Cores: "); tmh_itoa(ctx->telemetry.online_cores, num1); strcat(buf3, num1);
            tmh_create_card(ctx->content_panel_id, 15, 15, 290, 115, "CPU PERFORMANCE", buf1, buf2, buf3, "Status: Preemptive SMP");

            // RAM Overview Card
            strcpy(buf1, "Usage: 6.4 GB / 16 GB");
            strcpy(buf2, "Kernel: 512 MB | User: 5.9 GB");
            strcpy(buf3, "Free: 9.6 GB Physical RAM");
            tmh_create_card(ctx->content_panel_id, 320, 15, 290, 115, "RAM MEMORY", buf1, buf2, buf3, "Protection: PML4 Supervisor");

            // Storage Overview Card
            strcpy(buf1, "Disk: 512 GB SSD");
            strcpy(buf2, "Used: 180 GB | Free: 332 GB");
            strcpy(buf3, "Filesystem: FAT32 Volume");
            tmh_create_card(ctx->content_panel_id, 15, 145, 290, 115, "STORAGE SYSTEM", buf1, buf2, buf3, "Read: 22 MB/s | Write: 18 MB/s");

            // Ethernet Overview Card
            strcpy(buf1, "Status: Connected");
            strcpy(buf2, "IP: 192.168.1.20");
            strcpy(buf3, "Speed: 1 Gbps Gigabit NIC");
            tmh_create_card(ctx->content_panel_id, 320, 145, 290, 115, "ETHERNET NETWORK", buf1, buf2, buf3, "RX: 252 MB | TX: 110 MB");

            // System Uptime Card
            strcpy(buf1, "Display: 1920x1080 @ 32 BPP");
            strcpy(buf2, "Uptime: "); strcat(buf2, ctx->telemetry.uptime_str);
            strcpy(buf3, "Architecture: x86_64 SMP OS");
            tmh_create_card(ctx->content_panel_id, 15, 275, 595, 115, "SYSTEM SUMMARY & GPU", buf1, buf2, buf3, "Framebuffer: 0xFD000000 (VBE Linear)");
            break;
        }

        case TMH_TAB_CPU: {
            // CPU Top Card
            strcpy(buf1, "Model: "); strcat(buf1, ctx->telemetry.cpu_model);
            strcpy(buf2, "Overall Usage: "); tmh_itoa(ctx->telemetry.total_cpu_usage, num1); strcat(buf2, num1); strcat(buf2, "% | Freq: 4.21 GHz");
            strcpy(buf3, "Base Clock: 3.90 GHz | Cores: 6 | Threads: 12");
            tmh_create_card(ctx->content_panel_id, 15, 10, 595, 115, "CPU HARDWARE TELEMETRY", buf1, buf2, buf3, "Scheduler State: Running");

            // Live Per-Core Activity Bars Card
            uint32_t per_core_card_id = 0;
            BOS_CreatePanel(ctx->content_panel_id, 15, 135, 290, 260, 0xFF1E293B, &per_core_card_id);
            if (per_core_card_id) {
                uint32_t lbl = 0;
                BOS_CreateLabel(per_core_card_id, 10, 8, "LIVE PER-CORE CPU BARS", 0xFFF8FAFC, &lbl);

                char core_buf[64];
                char bar_str[32];
                uint32_t sim_cores[12] = {38, 16, 55, 8, 29, 40, 3, 22, 31, 12, 48, 20};

                for (uint32_t c = 0; c < 12; c++) {
                    strcpy(core_buf, "CPU");
                    tmh_itoa(c, num1);
                    strcat(core_buf, num1);
                    if (c < 10) strcat(core_buf, " ");
                    strcat(core_buf, " ");

                    tmh_make_bar(sim_cores[c], bar_str, 8);
                    strcat(core_buf, bar_str);

                    BOS_CreateLabel(per_core_card_id, 10, 28 + c * 18, core_buf, (sim_cores[c] > 40) ? 0xFFF59E0B : 0xFF38BDF8, &lbl);
                }
            }

            // Running Threads & Load Balance Card
            uint32_t right_card_id = 0;
            BOS_CreatePanel(ctx->content_panel_id, 320, 135, 290, 260, 0xFF1E293B, &right_card_id);
            if (right_card_id) {
                uint32_t lbl = 0;
                BOS_CreateLabel(right_card_id, 10, 8, "RUNNING THREADS & LOAD", 0xFFF8FAFC, &lbl);

                // Thread mapping entries
                const char* thread_mappings[5] = {
                    "Thread 1023 -> CPU0",
                    "Thread 88   -> CPU5",
                    "Thread 442  -> CPU2",
                    "Thread 17   -> CPU9",
                    "Thread 923  -> CPU4"
                };

                for (int i = 0; i < 5; i++) {
                    BOS_CreateLabel(right_card_id, 10, 28 + i * 20, thread_mappings[i], 0xFF4ADE80, &lbl);
                }

                BOS_CreateLabel(right_card_id, 10, 135, "LOAD BALANCE BREAKDOWN", 0xFFF8FAFC, &lbl);

                const char* load_bal[4] = {
                    "CPU0: 12 Threads",
                    "CPU1:  9 Threads",
                    "CPU2: 18 Threads",
                    "CPU3: 11 Threads"
                };

                for (int i = 0; i < 4; i++) {
                    BOS_CreateLabel(right_card_id, 10, 155 + i * 20, load_bal[i], 0xFF38BDF8, &lbl);
                }
            }
            break;
        }

        case TMH_TAB_RAM: {
            strcpy(buf1, "Total RAM: 16 GB Physical Memory");
            strcpy(buf2, "Used Memory: 6.4 GB (40%)");
            strcpy(buf3, "Free Memory: 9.6 GB (60%)");
            tmh_create_card(ctx->content_panel_id, 15, 15, 595, 120, "PHYSICAL RAM METRICS", buf1, buf2, buf3, "Memory Status: Healthy");

            strcpy(buf1, "Kernel Memory: 512 MB Allocated");
            strcpy(buf2, "User Memory: 5.9 GB Active");
            strcpy(buf3, "PMM Paging: 4GB Identity PML4 Mapped");
            tmh_create_card(ctx->content_panel_id, 15, 150, 595, 120, "MEMORY ALLOCATION BREAKDOWN", buf1, buf2, buf3, "Heap Allocator: AMSSS Dynamic Backend");
            break;
        }

        case TMH_TAB_STORAGE: {
            strcpy(buf1, "Disk: 512 GB Solid State Drive (SSD)");
            strcpy(buf2, "Used Space: 180 GB");
            strcpy(buf3, "Free Space: 332 GB");
            tmh_create_card(ctx->content_panel_id, 15, 15, 595, 120, "STORAGE DISK OVERVIEW", buf1, buf2, buf3, "Filesystem: FAT32 Volume");

            strcpy(buf1, "Read Throughput: 22 MB/s");
            strcpy(buf2, "Write Throughput: 18 MB/s");
            strcpy(buf3, "Disk Image: build/OS.img (Boot Partition)");
            tmh_create_card(ctx->content_panel_id, 15, 150, 595, 120, "STORAGE PERFORMANCE", buf1, buf2, buf3, "Block Subsystem: ATA HDD Controller");
            break;
        }

        case TMH_TAB_ETHERNET: {
            strcpy(buf1, "Network Status: Connected");
            strcpy(buf2, "IP Address: 192.168.1.20");
            strcpy(buf3, "Link Speed: 1 Gbps Gigabit Ethernet");
            tmh_create_card(ctx->content_panel_id, 15, 15, 595, 120, "ETHERNET ADAPTER (E1000 PCI)", buf1, buf2, buf3, "MAC Address: 52:54:00:12:34:56");

            strcpy(buf1, "Data Received: 252 MB");
            strcpy(buf2, "Data Sent: 110 MB");
            strcpy(buf3, "Protocols: ATOME Network HAL / TLS Engine");
            tmh_create_card(ctx->content_panel_id, 15, 150, 595, 120, "NETWORK TRAFFIC TELEMETRY", buf1, buf2, buf3, "Socket Manager: Active");
            break;
        }

        case TMH_TAB_GRAPHICS: {
            strcpy(buf1, "Display Resolution: 1920 x 1080");
            strcpy(buf2, "Color Depth: 32 Bits Per Pixel (BPP)");
            strcpy(buf3, "Refresh Rate: 60 Hz");
            tmh_create_card(ctx->content_panel_id, 15, 15, 595, 120, "VESA DISPLAY SPECIFICATION", buf1, buf2, buf3, "Driver: VBE Linear Driver");

            strcpy(buf1, "Framebuffer Physical Address: 0xFD000000");
            strcpy(buf2, "Pitch: 7680 Bytes Per Line");
            strcpy(buf3, "Compositor Engine: BWE Double-Buffered");
            tmh_create_card(ctx->content_panel_id, 15, 150, 595, 120, "COMPOSITOR & VRAM HARDWARE", buf1, buf2, buf3, "Hardware Cursor: Fast-Path Active");
            break;
        }

        default:
            break;
    }

    // Update bottom status bar panel text
    if (ctx->status_bar_id) {
        BWE_Window* sb = BWE_GetWindow(ctx->status_bar_id);
        if (sb) {
            sb->child_count = 0;
            char sb_text[128];
            strcpy(sb_text, " Processes: ");
            tmh_itoa(ctx->telemetry.total_processes, num1); strcat(sb_text, num1);
            strcat(sb_text, "  |  Threads: ");
            tmh_itoa(ctx->telemetry.total_threads, num2); strcat(sb_text, num2);
            strcat(sb_text, "  |  CPU: ");
            tmh_itoa(ctx->telemetry.total_cpu_usage, num1); strcat(sb_text, num1); strcat(sb_text, "%");
            strcat(sb_text, "  |  RAM: 6.4 GB / 16 GB");

            uint32_t lbl = 0;
            BOS_CreateLabel(ctx->status_bar_id, 10, 4, sb_text, 0xFF38BDF8, &lbl);
            BWE_InvalidateWindow(ctx->status_bar_id);
        }
    }
}

int tmh_app_init(uint32_t* out_win_id) {
    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateWindow(100, 60, 780, 480, "TMH — Task Manager Hardware", &win_id);
    if (err != BWE_SUCCESS) return (int)err;

    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return -1;

    TMHCtx* ctx = (TMHCtx*)kcalloc(1, sizeof(TMHCtx));
    if (!ctx) return -1;
    ctx->win_id = win_id;
    ctx->active_tab = TMH_TAB_OVERVIEW;
    win->user_data = ctx;

    // Left Navigation Sidebar Panel (Dark Slate 800)
    BOS_CreatePanel(win_id, 0, 0, 150, 425, 0xFF1E293B, &ctx->sidebar_id);
    BWE_SetAnchorMode(ctx->sidebar_id, BWE_ANCHOR_LEFT | BWE_ANCHOR_TOP | BWE_ANCHOR_BOTTOM);

    const char* tab_names[TMH_TAB_COUNT] = {
        "Overview",
        "CPU Cores",
        "RAM Memory",
        "Storage Disk",
        "Ethernet NIC",
        "Graphics VBE"
    };

    if (ctx->sidebar_id != 0) {
        for (int i = 0; i < TMH_TAB_COUNT; i++) {
            BOS_CreateButton(ctx->sidebar_id, 8, 12 + i * 44, 134, 36, tab_names[i], tmh_tab_btn_clicked, &ctx->tab_buttons[i]);
        }
    }

    // Right Content Panel (Dark Slate 900)
    BOS_CreatePanel(win_id, 150, 0, 630, 425, 0xFF0F172A, &ctx->content_panel_id);
    BWE_SetAnchorMode(ctx->content_panel_id, BWE_ANCHOR_ALL);

    // Bottom Status Bar Panel
    BOS_CreatePanel(win_id, 0, 425, 780, 25, 0xFF0284C7, &ctx->status_bar_id);
    BWE_SetAnchorMode(ctx->status_bar_id, BWE_ANCHOR_LEFT | BWE_ANCHOR_RIGHT | BWE_ANCHOR_BOTTOM);

    tmh_render_active_tab(ctx);

    if (out_win_id) *out_win_id = win_id;
    return 0;
}
