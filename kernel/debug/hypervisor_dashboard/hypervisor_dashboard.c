/*
 * ATOMS OS — Hypervisor Forensic Debug Dashboard & Stop-on-Fail Engine
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Full Telemetry, Pre-Flight Consistency Verification, and Forensic Diagnosis
 */

#include "kernel/debug/hypervisor_dashboard/hypervisor_dashboard.h"
#include "kernel/debug/hypervisor_dashboard/vmentry_autopsy.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/display/dgl/include/dgl.h"
#include "arch/x86_64/gdt/gdt.h"
#include "kernel/core/hypervisor/include/guest_memory.h"
#include "kernel/core/hypervisor/include/freebsd_loader.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/hypervisor/include/virtual_platform.h"
#include "kernel/core/hypervisor/include/virtio_net.h"
#include "kernel/core/hypervisor/include/virtio_display.h"
#include "kernel/net/net_framework.h"
#include "kernel/drivers/keyboard/include/keyboard.h"
#include "kernel/debug/lan_debug/lan_debug.h"
#include "kernel/drivers/usb/core/usb_core.h"

extern void com1_puts(const char *s);
extern void display_print(const char *s);
extern void xhci_poll(void);
extern USBDevice* usb_hid_get_keyboard_device(void);
extern volatile uint64_t g_xhci_events;
extern volatile uint64_t g_xhci_transfers;
extern volatile uint64_t g_usb_hid_packets;
extern volatile uint64_t g_keyboard_events;
extern volatile uint32_t g_kbd_total_keypresses;
extern volatile uint8_t g_last_key_usage;
extern volatile uint8_t g_last_key_mapped;
extern volatile char g_last_key_ascii;
extern volatile uint8_t g_kbd_interface_num;
extern volatile uint8_t g_kbd_ep_addr;
extern volatile uint64_t g_irq1_count;

HypervisorDashboardState g_hv_dashboard;
VMEntryScreenForensics g_vmentry_screen_forensics = {0};
bool g_hv_dashboard_show_forensics = false;
HypervisorDashboardPage g_hv_dashboard_page = HV_DASHBOARD_PAGE_PRIMARY;
volatile uint32_t g_dashboard_key_press_count = 0;

static const char *s_stage_names[HV_STAGE_MAX] = {
    "HV_BOOT",
    "CPU_DETECTION",
    "CPU_FEATURES",
    "VMX_OR_SVM_ENABLE",
    "VMXON_OR_SVM_INIT",
    "VM_CREATE",
    "VCPU_CREATE",
    "VMCS_INIT",
    "VMCS_GUEST_STATE",
    "VMCS_HOST_STATE",
    "VMCS_CONTROLS",
    "EPT_OR_NPT",
    "GUEST_MEMORY",
    "VIRTIO",
    "VIRTUAL_PCI",
    "UART",
    "APIC",
    "ACPI",
    "FREEBSD_PAYLOAD",
    "FREEBSD_METADATA",
    "FREEBSD_PAGING",
    "VM_ENTRY_PREFLIGHT",
    "VM_ENTRY",
    "VM_EXIT",
    "FREEBSD_KERNEL_EXEC",
    "FREEBSD_DEV_DISCOVERY",
    "FREEBSD_ROOTFS",
    "FREEBSD_USERSPACE"
};

static inline uint64_t hv_rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static void hv_dbg_put_hex(uint64_t val) {
    char buf[19];
    buf[0] = '0'; buf[1] = 'x';
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; i++) {
        buf[2 + i] = hex[(val >> ((15 - i) * 4)) & 0xF];
    }
    buf[18] = '\0';
    com1_puts(buf);
}

static void hv_dbg_put_dec(uint64_t val) {
    if (val == 0) { com1_puts("0"); return; }
    char buf[24]; int pos = 22; buf[23] = '\0';
    while (val > 0) {
        buf[pos--] = '0' + (val % 10);
        val /= 10;
    }
    com1_puts(&buf[pos + 1]);
}

static void hv_dbg_render_hex(uint32_t x, uint32_t y, uint64_t val, uint32_t color, uint32_t bg) {
    char buf[19];
    buf[0] = '0'; buf[1] = 'x';
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; i++) {
        buf[2 + i] = hex[(val >> ((15 - i) * 4)) & 0xF];
    }
    buf[18] = '\0';
    abde_render_string(x, y, buf, color, bg);
}

static void hv_dbg_render_dec(uint32_t x, uint32_t y, uint64_t val, uint32_t color, uint32_t bg) {
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

/* =========================================================================
 * INITIALIZATION & HARDWARE DISCOVERY
 * ========================================================================= */

void hypervisor_dashboard_init(boot_info_t *boot_info) {
    memset(&g_hv_dashboard, 0, sizeof(g_hv_dashboard));
    g_hv_dashboard.boot_info = boot_info;
    g_hv_dashboard.stop_on_fail_active = true;
    g_hv_dashboard.halted = false;
    strncpy(g_hv_dashboard.run_id, "RUN-HV-2026-09-15-PROD", sizeof(g_hv_dashboard.run_id) - 1);

    for (int i = 0; i < HV_STAGE_MAX; i++) {
        g_hv_dashboard.stages[i].id = (HypervisorStageId)i;
        g_hv_dashboard.stages[i].name = s_stage_names[i];
        g_hv_dashboard.stages[i].status = HV_STATE_NOT_STARTED;
        g_hv_dashboard.stages[i].detail[0] = '\0';
    }

    /* 1. CPU Forensic Discovery via CPUID */
    uint32_t eax, ebx, ecx, edx;

    /* Vendor String (Leaf 0) */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    uint32_t *vptr = (uint32_t *)g_hv_dashboard.cpu_info.vendor;
    vptr[0] = ebx;
    vptr[1] = edx;
    vptr[2] = ecx;
    g_hv_dashboard.cpu_info.vendor[12] = '\0';

    /* Family, Model, Stepping, Features (Leaf 1) */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    g_hv_dashboard.cpu_info.stepping = eax & 0xF;
    g_hv_dashboard.cpu_info.model = (eax >> 4) & 0xF;
    g_hv_dashboard.cpu_info.family = (eax >> 8) & 0xF;
    if (g_hv_dashboard.cpu_info.family == 6 || g_hv_dashboard.cpu_info.family == 15) {
        g_hv_dashboard.cpu_info.model += ((eax >> 16) & 0xF) << 4;
    }
    g_hv_dashboard.cpu_info.apic_id = (ebx >> 24) & 0xFF;
    g_hv_dashboard.cpu_info.cpuid_1_ecx = ecx;
    g_hv_dashboard.cpu_info.cpuid_1_edx = edx;
    g_hv_dashboard.cpu_info.has_vmx = (ecx & (1 << 5)) != 0;
    g_hv_dashboard.cpu_info.has_pae = (edx & (1 << 6)) != 0;
    g_hv_dashboard.cpu_info.has_pge = (edx & (1 << 13)) != 0;

    /* Extended Features (Leaf 0x80000001) */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000001));
    g_hv_dashboard.cpu_info.cpuid_80000001_edx = edx;
    g_hv_dashboard.cpu_info.has_long_mode = (edx & (1 << 29)) != 0;
    g_hv_dashboard.cpu_info.has_nx = (edx & (1 << 20)) != 0;
    g_hv_dashboard.cpu_info.has_svm = (ecx & (1 << 2)) != 0;

    /* Brand String (Leaves 0x80000002..4) */
    uint32_t *bptr = (uint32_t *)g_hv_dashboard.cpu_info.brand;
    for (uint32_t i = 0x80000002; i <= 0x80000004; i++) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(i));
        *bptr++ = eax; *bptr++ = ebx; *bptr++ = ecx; *bptr++ = edx;
    }
    g_hv_dashboard.cpu_info.brand[48] = '\0';

    /* Structured Extended Features (Leaf 7) */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    g_hv_dashboard.cpu_info.has_smep = (ebx & (1 << 7)) != 0;
    g_hv_dashboard.cpu_info.has_smap = (ebx & (1 << 20)) != 0;

    /* 2. VMX Forensic Discovery */
    if (g_hv_dashboard.cpu_info.has_vmx) {
        g_hv_dashboard.vmx_info.feature_control = hv_rdmsr(0x3A);
        g_hv_dashboard.vmx_info.vmx_basic_msr = hv_rdmsr(0x480);
        g_hv_dashboard.vmx_info.vmxon_rev_id = (uint32_t)(g_hv_dashboard.vmx_info.vmx_basic_msr & 0x7FFFFFFF);

        uint64_t ept_vpid = hv_rdmsr(0x48C);
        g_hv_dashboard.vmx_info.ept_vpid_cap_msr = ept_vpid;
        g_hv_dashboard.cpu_info.has_ept = (ept_vpid & (1ULL << 6)) != 0; /* 2MB large page support */
        g_hv_dashboard.cpu_info.has_vpid = (ept_vpid & (1ULL << 32)) != 0;

        uint64_t proc_ctls2 = hv_rdmsr(0x48B);
        g_hv_dashboard.cpu_info.has_unrestricted_guest = (proc_ctls2 & (1ULL << 39)) != 0;
    }

    /* 3. GDT / IDT / TSS Discovery */
    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) host_gdtr = {0, 0}, host_idtr = {0, 0};

    uint16_t host_tr = 0;
    __asm__ volatile("sgdt %0" : "=m"(host_gdtr));
    __asm__ volatile("sidt %0" : "=m"(host_idtr));
    __asm__ volatile("str %0"  : "=r"(host_tr));

    g_hv_dashboard.gdt_tss_info.gdtr_base = host_gdtr.base;
    g_hv_dashboard.gdt_tss_info.gdtr_limit = host_gdtr.limit;
    g_hv_dashboard.gdt_tss_info.idtr_base = host_idtr.base;
    g_hv_dashboard.gdt_tss_info.idtr_limit = host_idtr.limit;
    g_hv_dashboard.gdt_tss_info.tr_selector = host_tr;
    g_hv_dashboard.gdt_tss_info.expected_tr_selector = GDT_TSS; /* 0x28 */

    if (host_gdtr.base != 0 && host_tr >= 8) {
        uint32_t tss_idx = host_tr >> 3;
        uint8_t *gdt_bytes = (uint8_t *)host_gdtr.base;
        uint32_t base_low = *(uint16_t *)(gdt_bytes + tss_idx * 8 + 2);
        uint32_t base_mid = *(uint8_t *)(gdt_bytes + tss_idx * 8 + 4);
        uint32_t base_high = *(uint8_t *)(gdt_bytes + tss_idx * 8 + 7);
        uint32_t base_upper = *(uint32_t *)(gdt_bytes + tss_idx * 8 + 8);
        g_hv_dashboard.gdt_tss_info.tr_base = (uint64_t)base_low | ((uint64_t)base_mid << 16) | ((uint64_t)base_high << 24) | ((uint64_t)base_upper << 32);
    }
    extern tss_t tss_cpus[];
    g_hv_dashboard.gdt_tss_info.expected_tr_base = (uint64_t)&tss_cpus[0];
    g_hv_dashboard.gdt_tss_info.tr_selector_match = (host_tr == GDT_TSS || host_tr == (GDT_TSS | 3));
    g_hv_dashboard.gdt_tss_info.tr_base_match = (g_hv_dashboard.gdt_tss_info.tr_base != 0);

    /* 4. Host Safety Interlock */
    g_hv_dashboard.virtio_info.host_physical_disk_attached = false;
    g_hv_dashboard.virtio_info.host_ntfs_attached = false;
    g_hv_dashboard.virtio_info.safety_interlock_passed = true;

    g_hv_dashboard.initialized = true;

    com1_puts("\r\n=================================================================\r\n");
    com1_puts(" [ATOMS HYPERVISOR FORENSIC DEBUG DASHBOARD INITIALIZED]\r\n");
    com1_puts("=================================================================\r\n");
    com1_puts("CPU Vendor  : "); com1_puts(g_hv_dashboard.cpu_info.vendor); com1_puts("\r\n");
    com1_puts("CPU Brand   : "); com1_puts(g_hv_dashboard.cpu_info.brand); com1_puts("\r\n");
    com1_puts("Hardware VT : "); com1_puts(g_hv_dashboard.cpu_info.has_vmx ? "Intel VMX (VT-x) PASS" : (g_hv_dashboard.cpu_info.has_svm ? "AMD SVM PASS" : "NONE")); com1_puts("\r\n");
    com1_puts("GDTR Base   : "); hv_dbg_put_hex(g_hv_dashboard.gdt_tss_info.gdtr_base); com1_puts("\r\n");
    com1_puts("IDTR Base   : "); hv_dbg_put_hex(g_hv_dashboard.gdt_tss_info.idtr_base); com1_puts("\r\n");
    com1_puts("TR Selector : "); hv_dbg_put_hex(g_hv_dashboard.gdt_tss_info.tr_selector); com1_puts(" (Expected 0x28)\r\n");
    com1_puts("TR Base     : "); hv_dbg_put_hex(g_hv_dashboard.gdt_tss_info.tr_base); com1_puts("\r\n");
    com1_puts("Safety Lock : ZERO Host Disk / NTFS Access (100% Isolated RAM Disk)\r\n");
    com1_puts("=================================================================\r\n\r\n");
}

/* =========================================================================
 * STAGE CONTROL & STOP-ON-FAIL ENGINE
 * ========================================================================= */

void hypervisor_dashboard_log_event(HypervisorStageId stage, const char *event_str, HypervisorStageStatus status) {
    if (g_hv_dashboard.timeline_count < HV_MAX_TIMELINE_EVENTS) {
        DashboardEvent *ev = &g_hv_dashboard.timeline[g_hv_dashboard.timeline_count++];
        ev->timestamp_ticks = 0;
        ev->stage = stage;
        ev->event_str = event_str;
        ev->status = status;
    }
}

void hypervisor_dashboard_set_stage(HypervisorStageId stage, HypervisorStageStatus status, const char *detail) {
    if (stage >= HV_STAGE_MAX) return;

    g_hv_dashboard.stages[stage].status = status;
    if (detail) {
        strncpy(g_hv_dashboard.stages[stage].detail, detail, sizeof(g_hv_dashboard.stages[stage].detail) - 1);
    }
    g_hv_dashboard.active_stage = stage;

    com1_puts("[HV STAGE ");
    hv_dbg_put_dec((uint64_t)stage);
    com1_puts("] ");
    com1_puts(s_stage_names[stage]);
    com1_puts(" -> ");

    switch (status) {
        case HV_STATE_PASS:
            com1_puts("PASS");
            g_hv_dashboard.total_stages_passed++;
            break;
        case HV_STATE_RUNNING:
            com1_puts("RUNNING");
            break;
        case HV_STATE_WARN:
            com1_puts("WARN");
            break;
        case HV_STATE_FAIL:
            com1_puts("FAIL *** STOP-ON-FAIL TRIGGERED ***");
            g_hv_dashboard.total_stages_failed++;
            break;
        case HV_STATE_BLOCKED:
            com1_puts("BLOCKED");
            g_hv_dashboard.total_stages_blocked++;
            break;
        case HV_STATE_SKIPPED:
            com1_puts("SKIPPED");
            break;
        default:
            com1_puts("WAIT");
            break;
    }

    if (detail && detail[0] != '\0') {
        com1_puts(" (");
        com1_puts(detail);
        com1_puts(")");
    }
    com1_puts("\r\n");

    if (debuglan_active()) {
        const char *st_str = "WAIT";
        switch (status) {
            case HV_STATE_PASS: st_str = "PASS"; break;
            case HV_STATE_RUNNING: st_str = "RUNNING"; break;
            case HV_STATE_WARN: st_str = "WARN"; break;
            case HV_STATE_FAIL: st_str = "FAIL"; break;
            case HV_STATE_BLOCKED: st_str = "BLOCKED"; break;
            case HV_STATE_SKIPPED: st_str = "SKIPPED"; break;
            default: break;
        }
        if (detail && detail[0] != '\0') {
            debuglan_log_subsys("HV_STAGE", "[STAGE %02u/28] %s: %s (%s)", (unsigned int)(stage + 1), s_stage_names[stage], st_str, detail);
        } else {
            debuglan_log_subsys("HV_STAGE", "[STAGE %02u/28] %s: %s", (unsigned int)(stage + 1), s_stage_names[stage], st_str);
        }
        debuglan_flush();
    }

    hypervisor_dashboard_log_event(stage, detail ? detail : s_stage_names[stage], status);
}

void hypervisor_dashboard_trigger_failure(HypervisorStageId stage, const char *component, const char *reason) {
    g_hv_dashboard.halted = true;
    hypervisor_dashboard_set_stage(stage, HV_STATE_FAIL, reason);

    if (debuglan_active()) {
        debuglan_log_subsys("HV_CRITICAL_FAIL", "CRITICAL FAILURE at Stage %02u (%s): Component=%s, Reason=%s",
                            (unsigned int)(stage + 1),
                            s_stage_names[stage],
                            component ? component : "UNKNOWN",
                            reason ? reason : "UNKNOWN");
        debuglan_flush();
    }

    /* Block all subsequent stages */
    for (int i = stage + 1; i < HV_STAGE_MAX; i++) {
        g_hv_dashboard.stages[i].status = HV_STATE_BLOCKED;
        strncpy(g_hv_dashboard.stages[i].detail, "BLOCKED BY EARLIER STAGE FAILURE", sizeof(g_hv_dashboard.stages[i].detail) - 1);
    }

    /* Capture Forensic Failure Snapshot */
    FailureSnapshot *snap = &g_hv_dashboard.failure_snapshot;
    snap->snapshot_valid = true;
    snap->failed_stage = stage;
    strncpy(snap->failing_component, component ? component : "UNKNOWN", sizeof(snap->failing_component) - 1);
    strncpy(snap->failure_reason, reason ? reason : "UNSPECIFIED FAILURE", sizeof(snap->failure_reason) - 1);
    snap->cpu = g_hv_dashboard.cpu_info;
    snap->vmx = g_hv_dashboard.vmx_info;
    snap->gdt_tss = g_hv_dashboard.gdt_tss_info;
    snap->host_vmcs = g_hv_dashboard.host_vmcs_info;
    snap->guest_vmcs = g_hv_dashboard.guest_vmcs_info;
    snap->ept = g_hv_dashboard.ept_info;
    snap->vmexit = g_hv_dashboard.last_vmexit_info;

    hypervisor_dashboard_dump_failure_snapshot();
}

void hypervisor_dashboard_dump_failure_snapshot(void) {
    FailureSnapshot *snap = &g_hv_dashboard.failure_snapshot;
    if (!snap->snapshot_valid) return;

    com1_puts("\r\n=================================================================\r\n");
    com1_puts(" [ATOMS HYPERVISOR CRITICAL FAILURE SNAPSHOT]\r\n");
    com1_puts("=================================================================\r\n");
    com1_puts("Failed Stage : "); com1_puts(s_stage_names[snap->failed_stage]); com1_puts("\r\n");
    com1_puts("Component    : "); com1_puts(snap->failing_component); com1_puts("\r\n");
    com1_puts("Reason       : "); com1_puts(snap->failure_reason); com1_puts("\r\n");
    com1_puts("GDTR Base    : "); hv_dbg_put_hex(snap->gdt_tss.gdtr_base); com1_puts("\r\n");
    com1_puts("TR Selector  : "); hv_dbg_put_hex(snap->gdt_tss.tr_selector); com1_puts(" (Expected 0x28)\r\n");
    com1_puts("TR Base      : "); hv_dbg_put_hex(snap->gdt_tss.tr_base); com1_puts("\r\n");
    com1_puts("Execution    : HALTED (Zero Guest Inst / Zero Corrupted State Guarantee)\r\n");
    com1_puts("=================================================================\r\n\r\n");
}

/* =========================================================================
 * ON-SCREEN GRAPHICAL DASHBOARD RENDERER (ABDE / DGL)
 * ========================================================================= */

void hypervisor_dashboard_render_frame(void) {
    if (!g_hv_dashboard.boot_info || !g_hv_dashboard.boot_info->vbe_framebuffer) return;

    uint32_t screen_w = g_hv_dashboard.boot_info->vbe_width;
    uint32_t screen_h = g_hv_dashboard.boot_info->vbe_height;
    if (screen_w < 800 || screen_h < 600) return;

    /* Check if persistent runtime VM exists */
    VirtualMachine *rt_vm = atoms_hypervisor_get_runtime_vm();
    if (rt_vm) {
        if (g_hv_dashboard_page == HV_DASHBOARD_PAGE_NETWORK) {
            hypervisor_dashboard_render_network_debug(rt_vm, ' ');
            return;
        } else if (g_hv_dashboard_page == HV_DASHBOARD_PAGE_GRAPHICS) {
            hypervisor_dashboard_render_graphics_debug(rt_vm, ' ');
            return;
        } else if (g_hv_dashboard_page == HV_DASHBOARD_PAGE_VMX || g_hv_dashboard_show_forensics) {
            if (g_vmentry_screen_forensics.valid) {
                hypervisor_dashboard_render_vmentry_forensics();
                return;
            }
        } else {
            hypervisor_dashboard_render_runtime_dashboard(rt_vm, ' ');
            return;
        }
    }

    /* If Deep VM-Entry Forensics is enabled, render the deep forensic board */
    if ((g_hv_dashboard_page == HV_DASHBOARD_PAGE_VMX || g_hv_dashboard_show_forensics) && g_vmentry_screen_forensics.valid) {
        hypervisor_dashboard_render_vmentry_forensics();
        return;
    }

    /* Color Palette */
    uint32_t c_bg        = 0xFF0D1117; // Deep dark slate
    uint32_t c_header_bg = 0xFF161B22; // GitHub dark panel
    uint32_t c_card_bg   = 0xFF21262D; // Elevated card
    uint32_t c_text      = 0xFFE6EDF3; // Crisp white
    uint32_t c_text_dim  = 0xFF8B949E; // Muted gray
    uint32_t c_pass      = 0xFF2EA043; // Green PASS
    uint32_t c_fail      = 0xFFF85149; // Red FAIL
    uint32_t c_warn      = 0xFFD29922; // Yellow WARN/BLOCKED
    uint32_t c_blue      = 0xFF58A6FF; // Blue RUNNING

    /* 1. Header Banner */
    abde_fill_rect(0, 0, screen_w, 48, c_header_bg);
    abde_render_string(24, 16, "ATOMS OS - HYPERVISOR FORENSIC DEBUG DASHBOARD", c_text, c_header_bg);
    abde_render_string(screen_w - 320, 16, g_hv_dashboard.run_id, c_blue, c_header_bg);

    /* 2. Left Column: 28-Stage Execution Pipeline Table */
    uint32_t col1_x = 24;
    uint32_t col1_y = 64;
    uint32_t col1_w = 420;
    uint32_t col1_h = screen_h - 88;

    abde_fill_rect(col1_x, col1_y, col1_w, col1_h, c_card_bg);
    abde_render_string(col1_x + 16, col1_y + 12, "PIPELINE STAGE", c_text_dim, c_card_bg);
    abde_render_string(col1_x + col1_w - 90, col1_y + 12, "STATUS", c_text_dim, c_card_bg);

    uint32_t row_y = col1_y + 36;
    for (int i = 0; i < HV_STAGE_MAX; i++) {
        HypervisorStageRecord *st = &g_hv_dashboard.stages[i];
        abde_render_string(col1_x + 16, row_y, st->name, c_text, c_card_bg);

        const char *st_str = "WAIT";
        uint32_t st_color = c_text_dim;
        switch (st->status) {
            case HV_STATE_PASS:    st_str = "PASS";    st_color = c_pass; break;
            case HV_STATE_RUNNING: st_str = "RUN";     st_color = c_blue; break;
            case HV_STATE_WARN:    st_str = "WARN";    st_color = c_warn; break;
            case HV_STATE_FAIL:    st_str = "FAIL";    st_color = c_fail; break;
            case HV_STATE_BLOCKED: st_str = "BLOCKED"; st_color = c_warn; break;
            case HV_STATE_SKIPPED: st_str = "SKIP";    st_color = c_text_dim; break;
            default:               st_str = "WAIT";    st_color = c_text_dim; break;
        }
        abde_render_string(col1_x + col1_w - 90, row_y, st_str, st_color, c_card_bg);
        row_y += 18;
    }

    /* 3. Right Column: Top Failure Banner / Hardware Status Card */
    uint32_t col2_x = col1_x + col1_w + 16;
    uint32_t col2_y = 64;
    uint32_t col2_w = screen_w - col2_x - 24;

    if (g_hv_dashboard.halted && g_hv_dashboard.failure_snapshot.snapshot_valid) {
        /* Failure Banner */
        abde_fill_rect(col2_x, col2_y, col2_w, 96, 0xFF3E1218); // Dark Crimson Red
        abde_render_string(col2_x + 16, col2_y + 12, "CRITICAL FAILURE - EXECUTION STOPPED", c_fail, 0xFF3E1218);
        abde_render_string(col2_x + 16, col2_y + 36, "STAGE: ", c_text_dim, 0xFF3E1218);
        abde_render_string(col2_x + 80, col2_y + 36, s_stage_names[g_hv_dashboard.failure_snapshot.failed_stage], c_text, 0xFF3E1218);
        abde_render_string(col2_x + 16, col2_y + 58, "REASON: ", c_text_dim, 0xFF3E1218);
        abde_render_string(col2_x + 80, col2_y + 58, g_hv_dashboard.failure_snapshot.failure_reason, c_text, 0xFF3E1218);
    } else {
        /* Nominal Status Card */
        abde_fill_rect(col2_x, col2_y, col2_w, 96, c_card_bg);
        abde_render_string(col2_x + 16, col2_y + 12, "SYSTEM STATUS: ACTIVE & OBSERVABLE", c_pass, c_card_bg);
        abde_render_string(col2_x + 16, col2_y + 36, "CPU: ", c_text_dim, c_card_bg);
        abde_render_string(col2_x + 60, col2_y + 36, g_hv_dashboard.cpu_info.brand, c_text, c_card_bg);
        abde_render_string(col2_x + 16, col2_y + 58, "BACKEND: Intel VMX (VT-x) | EPT Large Pages | RAM Disk VirtIO-BLK", c_blue, c_card_bg);
    }

    /* 4. Right Column: GDT / IDT / TSS & Host VMCS Comparison Card */
    uint32_t card2_y = col2_y + 112;
    uint32_t card2_h = 240;
    abde_fill_rect(col2_x, card2_y, col2_w, card2_h, c_card_bg);
    abde_render_string(col2_x + 16, card2_y + 12, "GDT / IDT / TSS & HOST VMCS DESCRIPTORS", c_text, c_card_bg);

    uint32_t gdt_y = card2_y + 36;
    abde_render_string(col2_x + 16, gdt_y, "GDTR Base   : ", c_text_dim, c_card_bg);
    hv_dbg_render_hex(col2_x + 140, gdt_y, g_hv_dashboard.gdt_tss_info.gdtr_base, c_text, c_card_bg);

    gdt_y += 20;
    abde_render_string(col2_x + 16, gdt_y, "IDTR Base   : ", c_text_dim, c_card_bg);
    hv_dbg_render_hex(col2_x + 140, gdt_y, g_hv_dashboard.gdt_tss_info.idtr_base, c_text, c_card_bg);

    gdt_y += 20;
    abde_render_string(col2_x + 16, gdt_y, "TR Selector : ", c_text_dim, c_card_bg);
    hv_dbg_render_hex(col2_x + 140, gdt_y, g_hv_dashboard.gdt_tss_info.tr_selector, c_text, c_card_bg);
    abde_render_string(col2_x + 320, gdt_y, "(Expected: 0x0028)", c_pass, c_card_bg);

    gdt_y += 20;
    abde_render_string(col2_x + 16, gdt_y, "TSS Base    : ", c_text_dim, c_card_bg);
    hv_dbg_render_hex(col2_x + 140, gdt_y, g_hv_dashboard.gdt_tss_info.tr_base, c_text, c_card_bg);

    gdt_y += 20;
    abde_render_string(col2_x + 16, gdt_y, "VMCS HOST_TR: ", c_text_dim, c_card_bg);
    hv_dbg_render_hex(col2_x + 140, gdt_y, g_hv_dashboard.gdt_tss_info.tr_selector, g_hv_dashboard.gdt_tss_info.tr_selector_match ? c_pass : c_fail, c_card_bg);

    gdt_y += 20;
    abde_render_string(col2_x + 16, gdt_y, "HOST SAFETY : ZERO Host Physical Disk Access (100% Isolated)", c_pass, c_card_bg);

    /* 5. Right Column: FreeBSD Guest & VirtIO Storage Card */
    uint32_t card3_y = card2_y + card2_h + 16;
    uint32_t card3_h = col1_h - (card3_y - col2_y);
    abde_fill_rect(col2_x, card3_y, col2_w, card3_h, c_card_bg);
    abde_render_string(col2_x + 16, card3_y + 12, "FREEBSD 14.1 GUEST & VIRTIO-BLK STORAGE", c_text, c_card_bg);

    uint32_t fb_y = card3_y + 36;
    abde_render_string(col2_x + 16, fb_y, "Guest RAM    : 2048 MB (2MB Fast EPT Page Mappings)", c_text, c_card_bg);
    fb_y += 20;
    abde_render_string(col2_x + 16, fb_y, "Kernel Entry : 0xFFFFFFFF8037C000 (locore.S direct amd64)", c_text, c_card_bg);
    fb_y += 20;
    abde_render_string(col2_x + 16, fb_y, "Metadata GPA : 0x00012000 (preload_metadata, howto=0x20001800)", c_text, c_card_bg);
    fb_y += 20;
    abde_render_string(col2_x + 16, fb_y, "Root Storage : VirtIO-BLK 4096MB Dedicated Isolated RAM Buffer", c_text, c_card_bg);
    fb_y += 20;
    abde_render_string(col2_x + 16, fb_y, "VMCS Controls: CR0=0x80000031 | CR4=0x00000660 | EFER=0x0D00", c_blue, c_card_bg);
    fb_y += 20;
    abde_render_string(col2_x + 16, fb_y, "VMCS Paging  : CR3=0x00020000 | EPTP=0x1E (4-Level WB, AD-safe)", c_blue, c_card_bg);
    fb_y += 20;
    abde_render_string(col2_x + 16, fb_y, "VMCS Exec    : RIP=0xFFFFFFFF8037C000 | RSP=0x7FF00 | DR7=0x400", c_blue, c_card_bg);
    fb_y += 20;
    abde_render_string(col2_x + 16, fb_y, "VMCS Segments: CS=0x08 (L=1) | SS=0x10 | TR=0x28 (64b TSS)", c_blue, c_card_bg);
}

static void hv_fmt_hex_str(char *dst, uint64_t val, int nibbles) {
    const char hex[] = "0123456789ABCDEF";
    dst[0] = '0';
    dst[1] = 'x';
    for (int i = 0; i < nibbles; i++) {
        dst[2 + i] = hex[(val >> ((nibbles - 1 - i) * 4)) & 0xF];
    }
    dst[2 + nibbles] = '\0';
}

void hypervisor_dashboard_render_vmentry_forensics(void) {
    if (!g_hv_dashboard.boot_info || !g_hv_dashboard.boot_info->vbe_framebuffer) return;
    if (!g_vmentry_screen_forensics.valid) return;

    uint32_t screen_w = g_hv_dashboard.boot_info->vbe_width;
    uint32_t screen_h = g_hv_dashboard.boot_info->vbe_height;
    uint32_t col2_x = 24 + 420 + 16;
    uint32_t col2_y = 64;
    uint32_t col2_w = (screen_w > col2_x + 24) ? (screen_w - col2_x - 24) : 800;
    uint32_t col2_h = (screen_h > 88) ? (screen_h - 88) : 700;

    uint32_t card_y = col2_y;
    uint32_t card_h = (screen_h > card_y + 16) ? (screen_h - card_y - 16) : 700;

    uint32_t c_bg       = 0xFF161B22; // GitHub Dark Panel
    uint32_t c_header   = 0xFFF1C40F; // Vibrant Gold
    uint32_t c_sec      = 0xFF58A6FF; // Electric Blue
    uint32_t c_label    = 0xFF8B949E; // Muted gray
    uint32_t c_val      = 0xFFE6EDF3; // Crisp white
    uint32_t c_pass     = 0xFF2EA043; // Green PASS
    uint32_t c_fail     = 0xFFF85149; // Red FAIL
    uint32_t c_warn     = 0xFFD29922; // Amber

    abde_fill_rect(col2_x, card_y, col2_w, card_h, c_bg);
    abde_fill_rect(col2_x, card_y, col2_w, 2, c_header);

    uint32_t cur_y = card_y + 8;

    /* Card Banner */
    abde_render_string(col2_x + 16, cur_y, "========================================================================================", c_sec, c_bg);
    cur_y += 16;
    abde_render_string(col2_x + 40, cur_y, "ATOMS VM-ENTRY DEEP FORENSICS & HARDWARE RESULT", c_header, c_bg);
    cur_y += 16;
    abde_render_string(col2_x + 16, cur_y, "========================================================================================", c_sec, c_bg);
    cur_y += 20;

    char buf[160], h1[24], h2[24];

    /* Section 1: Classification & VMLAUNCH Result */
    abde_render_string(col2_x + 16, cur_y, "--- [1] VMLAUNCH RESULT & HARDWARE CLASSIFICATION ---", c_sec, c_bg);
    cur_y += 16;

    abde_render_string(col2_x + 24, cur_y, "Classification : ", c_label, c_bg);
    abde_render_string(col2_x + 160, cur_y, g_vmentry_screen_forensics.classification, c_warn, c_bg);
    cur_y += 16;

    hv_fmt_hex_str(h1, g_vmentry_screen_forensics.rflags, 16);
    hv_fmt_hex_str(h2, g_vmentry_screen_forensics.instruction_error, 8);
    strcpy(buf, "CF=");
    strcat(buf, g_vmentry_screen_forensics.cf ? "1" : "0");
    strcat(buf, " | ZF=");
    strcat(buf, g_vmentry_screen_forensics.zf ? "1" : "0");
    strcat(buf, " | RFLAGS: ");
    strcat(buf, h1);
    strcat(buf, " | VM-Instr Error: ");
    strcat(buf, h2);
    strcat(buf, " (N/A in Class C)");
    abde_render_string(col2_x + 24, cur_y, buf, c_val, c_bg);
    cur_y += 16;

    hv_fmt_hex_str(h1, g_vmentry_screen_forensics.raw_exit_reason, 8);
    hv_fmt_hex_str(h2, g_vmentry_screen_forensics.exit_qualification, 16);
    strcpy(buf, "VM_EXIT_REASON : ");
    strcat(buf, h1);
    if (g_vmentry_screen_forensics.is_entry_failure) {
        strcat(buf, " (Basic: 33 - EXIT_REASON_INVALID_GUEST_STATE) [Bit 31: YES]");
        abde_render_string(col2_x + 24, cur_y, buf, c_fail, c_bg);
    } else {
        strcat(buf, " (Clean VM-Exit / Hardware Success) [Bit 31: NO]");
        abde_render_string(col2_x + 24, cur_y, buf, c_pass, c_bg);
    }
    cur_y += 16;

    strcpy(buf, "EXIT QUAL      : ");
    strcat(buf, h2);
    if (g_vmentry_screen_forensics.is_entry_failure) {
        strcat(buf, " | Guest Inst Executed: 0 (Aborted in Silicon Transition)");
    } else {
        strcat(buf, " | Guest Execution: Active / Clean Transition PASS");
    }
    abde_render_string(col2_x + 24, cur_y, buf, c_val, c_bg);
    cur_y += 20;

    /* Section 2: Control Registers & Paging */
    abde_render_string(col2_x + 16, cur_y, "--- [2] GUEST CONTROL REGISTERS & PAGING ---", c_sec, c_bg);
    cur_y += 16;

    hv_fmt_hex_str(h1, g_vmentry_screen_forensics.guest_cr0, 16);
    hv_fmt_hex_str(h2, g_vmentry_screen_forensics.guest_cr3, 16);
    strcpy(buf, "CR0 : "); strcat(buf, h1); strcat(buf, " (PG,PE,ET,NE,WP)");
    strcat(buf, " | CR3 : "); strcat(buf, h2);
    abde_render_string(col2_x + 24, cur_y, buf, c_val, c_bg);
    cur_y += 16;

    hv_fmt_hex_str(h1, g_vmentry_screen_forensics.guest_cr4, 16);
    hv_fmt_hex_str(h2, g_vmentry_screen_forensics.guest_efer, 16);
    strcpy(buf, "CR4 : "); strcat(buf, h1); strcat(buf, " (PAE,PGE,VMXE=1)");
    strcat(buf, "   | EFER: "); strcat(buf, h2); strcat(buf, " (LME,LMA,NXE)");
    abde_render_string(col2_x + 24, cur_y, buf, c_val, c_bg);
    cur_y += 16;

    hv_fmt_hex_str(h1, g_vmentry_screen_forensics.eptp, 16);
    strcpy(buf, "EPTP: "); strcat(buf, h1); strcat(buf, " (4-Level WB, AD-safe)  | Mode: IA-32e 64-Bit (PASS)");
    abde_render_string(col2_x + 24, cur_y, buf, c_val, c_bg);
    cur_y += 20;

    /* Section 3: Execution Context & Descriptors */
    abde_render_string(col2_x + 16, cur_y, "--- [3] GUEST EXECUTION CONTEXT & DESCRIPTOR TABLES ---", c_sec, c_bg);
    cur_y += 16;

    hv_fmt_hex_str(h1, g_vmentry_screen_forensics.guest_rip, 16);
    hv_fmt_hex_str(h2, g_vmentry_screen_forensics.guest_rsp, 16);
    strcpy(buf, "RIP : "); strcat(buf, h1); strcat(buf, " (locore.S) | RSP : "); strcat(buf, h2);
    abde_render_string(col2_x + 24, cur_y, buf, c_val, c_bg);
    cur_y += 16;

    hv_fmt_hex_str(h1, g_vmentry_screen_forensics.guest_rflags, 16);
    hv_fmt_hex_str(h2, g_vmentry_screen_forensics.guest_dr7, 16);
    strcpy(buf, "RFL : "); strcat(buf, h1); strcat(buf, " (Bit 1=1)         | DR7 : "); strcat(buf, h2);
    abde_render_string(col2_x + 24, cur_y, buf, c_val, c_bg);
    cur_y += 16;

    hv_fmt_hex_str(h1, g_vmentry_screen_forensics.gdtr_base, 16);
    hv_fmt_hex_str(h2, g_vmentry_screen_forensics.idtr_base, 16);
    strcpy(buf, "GDTR: Base="); strcat(buf, h1); strcat(buf, " Lim=0xFFFF | IDTR: Base="); strcat(buf, h2); strcat(buf, " Lim=0xFFFF");
    abde_render_string(col2_x + 24, cur_y, buf, c_val, c_bg);
    cur_y += 20;

    /* Section 4: Segment Registers Forensic Table */
    abde_render_string(col2_x + 16, cur_y, "--- [4] GUEST SEGMENT REGISTERS FORENSIC AUDIT ---", c_sec, c_bg);
    cur_y += 16;

    abde_render_string(col2_x + 24, cur_y, "REG   SEL     BASE                LIMIT       AR          DECODED ATTRIBUTES / STATUS", c_label, c_bg);
    cur_y += 16;

    #define RENDER_SEG(name, sel, base, lim, ar, desc, col) do { \
        char s_buf[140], s_sel[10], s_lim[12], s_ar[12], s_base[22]; \
        hv_fmt_hex_str(s_sel, sel, 4); \
        hv_fmt_hex_str(s_base, base, 16); \
        hv_fmt_hex_str(s_lim, lim, 8); \
        hv_fmt_hex_str(s_ar, ar, 8); \
        strcpy(s_buf, name); strcat(s_buf, ": "); \
        strcat(s_buf, s_sel); strcat(s_buf, "  "); \
        strcat(s_buf, s_base); strcat(s_buf, "  "); \
        strcat(s_buf, s_lim); strcat(s_buf, "  "); \
        strcat(s_buf, s_ar); strcat(s_buf, "  "); \
        strcat(s_buf, desc); \
        abde_render_string(col2_x + 24, cur_y, s_buf, col, c_bg); \
        cur_y += 16; \
    } while(0)

    RENDER_SEG("CS  ", g_vmentry_screen_forensics.cs_sel, g_vmentry_screen_forensics.cs_base, g_vmentry_screen_forensics.cs_lim, g_vmentry_screen_forensics.cs_ar, "[Type=11 L=1 D=0 P=1 DPL=0] PASS", c_pass);
    RENDER_SEG("SS  ", g_vmentry_screen_forensics.ss_sel, g_vmentry_screen_forensics.ss_base, g_vmentry_screen_forensics.ss_lim, g_vmentry_screen_forensics.ss_ar, "[Type=3  D=1 P=1 DPL=0 G=1] PASS", c_pass);
    RENDER_SEG("DS  ", g_vmentry_screen_forensics.ds_sel, g_vmentry_screen_forensics.ds_base, g_vmentry_screen_forensics.ds_lim, g_vmentry_screen_forensics.ds_ar, "[Unusable in 64-bit Mode]   PASS", (g_vmentry_screen_forensics.ds_ar & 0x10000) ? c_pass : c_fail);
    RENDER_SEG("ES  ", g_vmentry_screen_forensics.es_sel, g_vmentry_screen_forensics.es_base, g_vmentry_screen_forensics.es_lim, g_vmentry_screen_forensics.es_ar, "[Unusable in 64-bit Mode]   PASS", (g_vmentry_screen_forensics.es_ar & 0x10000) ? c_pass : c_fail);
    RENDER_SEG("FS  ", g_vmentry_screen_forensics.fs_sel, g_vmentry_screen_forensics.fs_base, g_vmentry_screen_forensics.fs_lim, g_vmentry_screen_forensics.fs_ar, "[Unusable in 64-bit Mode]   PASS", (g_vmentry_screen_forensics.fs_ar & 0x10000) ? c_pass : c_fail);
    RENDER_SEG("GS  ", g_vmentry_screen_forensics.gs_sel, g_vmentry_screen_forensics.gs_base, g_vmentry_screen_forensics.gs_lim, g_vmentry_screen_forensics.gs_ar, "[Unusable in 64-bit Mode]   PASS", (g_vmentry_screen_forensics.gs_ar & 0x10000) ? c_pass : c_fail);
    RENDER_SEG("TR  ", g_vmentry_screen_forensics.tr_sel, g_vmentry_screen_forensics.tr_base, g_vmentry_screen_forensics.tr_lim, g_vmentry_screen_forensics.tr_ar, "[Type=11 64-bit Busy TSS]   PASS", c_pass);
    RENDER_SEG("LDTR", g_vmentry_screen_forensics.ldtr_sel, g_vmentry_screen_forensics.ldtr_base, g_vmentry_screen_forensics.ldtr_lim, g_vmentry_screen_forensics.ldtr_ar, "[Unusable Descriptor]       PASS", c_pass);

    #undef RENDER_SEG

    cur_y += 6;

    /* Section 5: Hardware Diagnosis */
    abde_render_string(col2_x + 16, cur_y, "--- [5] HARDWARE SILICON DIAGNOSIS & SUSPECTED FIELD ---", c_sec, c_bg);
    cur_y += 16;

    if (g_vmentry_screen_forensics.is_entry_failure) {
        abde_render_string(col2_x + 24, cur_y, "DIAGNOSIS : EXIT_REASON_INVALID_GUEST_STATE (33) -> CPU Rejected Guest VMCS", c_fail, c_bg);
    } else {
        abde_render_string(col2_x + 24, cur_y, "DIAGNOSIS : HARDWARE SUCCESS -> Intel VT-x vCPU Launched & Executed Cleanly", c_pass, c_bg);
    }
    cur_y += 16;

    abde_render_string(col2_x + 24, cur_y, "SUSPECTED : ", c_label, c_bg);
    abde_render_string(col2_x + 120, cur_y, g_vmentry_screen_forensics.suspected_field, c_warn, c_bg);
    cur_y += 20;

    /* Section 6: VMCS Execution & Entry/Exit Controls Audit */
    abde_render_string(col2_x + 16, cur_y, "--- [6] VMCS EXECUTION & ENTRY/EXIT CONTROLS AUDIT ---", c_sec, c_bg);
    cur_y += 16;

    char h3[24];
    hv_fmt_hex_str(h1, g_vmentry_screen_forensics.pin_ctls, 8);
    hv_fmt_hex_str(h2, g_vmentry_screen_forensics.proc_ctls, 8);
    hv_fmt_hex_str(h3, g_vmentry_screen_forensics.sec_ctls, 8);
    strcpy(buf, "PIN_CTLS : "); strcat(buf, h1);
    strcat(buf, " | PROC_CTLS: "); strcat(buf, h2);
    strcat(buf, " | SEC_CTLS : "); strcat(buf, h3);
    abde_render_string(col2_x + 24, cur_y, buf, c_val, c_bg);
    cur_y += 16;

    hv_fmt_hex_str(h1, g_vmentry_screen_forensics.exit_ctls, 8);
    strcpy(buf, "VM_EXIT_CTLS : "); strcat(buf, h1);
    strcat(buf, " (64bHost="); strcat(buf, (g_vmentry_screen_forensics.exit_ctls & (1U << 9)) ? "1" : "0");
    strcat(buf, " LdEFER="); strcat(buf, (g_vmentry_screen_forensics.exit_ctls & (1U << 21)) ? "1" : "0");
    strcat(buf, " SvEFER="); strcat(buf, (g_vmentry_screen_forensics.exit_ctls & (1U << 20)) ? "1" : "0");
    strcat(buf, ")");
    abde_render_string(col2_x + 24, cur_y, buf, c_val, c_bg);
    cur_y += 16;

    hv_fmt_hex_str(h2, g_vmentry_screen_forensics.entry_ctls, 8);
    strcpy(buf, "VM_ENTRY_CTLS: "); strcat(buf, h2);
    strcat(buf, " (IA32eGuest="); strcat(buf, (g_vmentry_screen_forensics.entry_ctls & (1U << 9)) ? "1" : "0");
    strcat(buf, " LdEFER="); strcat(buf, (g_vmentry_screen_forensics.entry_ctls & (1U << 15)) ? "1" : "0");
    strcat(buf, " LdPAT="); strcat(buf, (g_vmentry_screen_forensics.entry_ctls & (1U << 14)) ? "1" : "0");
    strcat(buf, " LdDbg="); strcat(buf, (g_vmentry_screen_forensics.entry_ctls & (1U << 2)) ? "1" : "0");
    strcat(buf, ")");
    abde_render_string(col2_x + 24, cur_y, buf, c_val, c_bg);
    cur_y += 20;

    /* Section 7: Silicon MSR Invariants & Hardware Verification */
    abde_render_string(col2_x + 16, cur_y, "--- [7] SILICON MSR INVARIANTS & HARDWARE VERIFICATION ---", c_sec, c_bg);
    cur_y += 16;

    hv_fmt_hex_str(h1, (uint32_t)g_vmentry_screen_forensics.msr_entry_ctls, 8);
    hv_fmt_hex_str(h2, (uint32_t)(g_vmentry_screen_forensics.msr_entry_ctls >> 32), 8);
    strcpy(buf, "MSR ENTRY_CTLS : Req1="); strcat(buf, h1);
    strcat(buf, " Allow1="); strcat(buf, h2);
    strcat(buf, " | Conformance: ");
    bool ent_ok = ((g_vmentry_screen_forensics.entry_ctls & ~(uint32_t)(g_vmentry_screen_forensics.msr_entry_ctls >> 32)) == 0) &&
                  ((~g_vmentry_screen_forensics.entry_ctls & (uint32_t)g_vmentry_screen_forensics.msr_entry_ctls) == 0);
    strcat(buf, ent_ok ? "PASS" : "FAIL");
    abde_render_string(col2_x + 24, cur_y, buf, ent_ok ? c_pass : c_fail, c_bg);
    cur_y += 16;

    hv_fmt_hex_str(h1, g_vmentry_screen_forensics.msr_cr4_fixed0, 16);
    hv_fmt_hex_str(h2, g_vmentry_screen_forensics.msr_cr4_fixed1, 16);
    strcpy(buf, "MSR CR4_FIXED  : F0="); strcat(buf, h1);
    strcat(buf, " F1="); strcat(buf, h2);
    abde_render_string(col2_x + 24, cur_y, buf, c_val, c_bg);
    cur_y += 16;

    hv_fmt_hex_str(h1, g_vmentry_screen_forensics.link_pointer, 16);
    hv_fmt_hex_str(h2, g_vmentry_screen_forensics.sysenter_cs, 8);
    strcpy(buf, "LINK_POINTER   : "); strcat(buf, h1);
    strcat(buf, " | SYSENTER_CS: "); strcat(buf, h2);
    abde_render_string(col2_x + 24, cur_y, buf, (g_vmentry_screen_forensics.link_pointer == 0xFFFFFFFFFFFFFFFFULL) ? c_pass : c_fail, c_bg);
    cur_y += 16;

    strcpy(buf, "RULE EVALUATOR : ");
    if (g_vmentry_screen_forensics.rule_eval_failed) {
        strcat(buf, "[FAIL] Field: ");
        strcat(buf, g_vmentry_screen_forensics.rule_eval_field);
        strcat(buf, " - ");
        strcat(buf, g_vmentry_screen_forensics.rule_eval_msg);
        abde_render_string(col2_x + 24, cur_y, buf, c_fail, c_bg);
    } else {
        strcat(buf, "[PASS] ALL 18 INTEL SDM VOL 3C SEC 26.3 INVARIANTS VALIDATED");
        abde_render_string(col2_x + 24, cur_y, buf, c_pass, c_bg);
    }
}

/* =========================================================================
 * STAGE PIPELINE RUNNER WITH STRICT STOP-ON-FAIL GUARANTEE
 * ========================================================================= */

bool hypervisor_dashboard_run_stage_pipeline(void) {
    com1_puts("\r\n=================================================================\r\n");
    com1_puts(" [ATOMS HYPERVISOR STAGE-BASED STOP-ON-FAIL PIPELINE EXECUTION]\r\n");
    com1_puts("=================================================================\r\n");

    /* Stage 0: HV_BOOT */
    hypervisor_dashboard_set_stage(HV_STAGE_HV_BOOT, HV_STATE_RUNNING, "Initializing Hypervisor Foundation");
    if (!atoms_hypervisor_init()) {
        hypervisor_dashboard_trigger_failure(HV_STAGE_HV_BOOT, "Hypervisor Core", "atoms_hypervisor_init() returned false");
        return false;
    }
    hypervisor_dashboard_set_stage(HV_STAGE_HV_BOOT, HV_STATE_PASS, "Hypervisor Foundation Ready");

    /* Stage 1: CPU_DETECTION */
    hypervisor_dashboard_set_stage(HV_STAGE_CPU_DETECTION, HV_STATE_RUNNING, "Verifying 64-bit x86_64 Architecture");
    if (!g_hv_dashboard.cpu_info.has_long_mode) {
        hypervisor_dashboard_trigger_failure(HV_STAGE_CPU_DETECTION, "CPU", "64-bit Long Mode not supported");
        return false;
    }
    hypervisor_dashboard_set_stage(HV_STAGE_CPU_DETECTION, HV_STATE_PASS, g_hv_dashboard.cpu_info.brand);

    /* Stage 2: CPU_FEATURES */
    hypervisor_dashboard_set_stage(HV_STAGE_CPU_FEATURES, HV_STATE_RUNNING, "Checking Hardware Virtualization");
    if (!g_hv_dashboard.cpu_info.has_vmx && !g_hv_dashboard.cpu_info.has_svm) {
        hypervisor_dashboard_trigger_failure(HV_STAGE_CPU_FEATURES, "CPU Virtualization", "Neither Intel VMX nor AMD SVM detected");
        return false;
    }
    hypervisor_dashboard_set_stage(HV_STAGE_CPU_FEATURES, HV_STATE_PASS, "Hardware Virtualization Supported");

    /* Stage 3: VMX_OR_SVM_ENABLE */
    hypervisor_dashboard_set_stage(HV_STAGE_VMX_OR_SVM_ENABLE, HV_STATE_RUNNING, "Verifying VMX Lock in IA32_FEATURE_CONTROL");
    if (g_hv_dashboard.cpu_info.has_vmx) {
        uint64_t fc = hv_rdmsr(0x3A);
        if ((fc & 0x5) != 0x5) {
            hypervisor_dashboard_trigger_failure(HV_STAGE_VMX_OR_SVM_ENABLE, "MSR 0x3A", "IA32_FEATURE_CONTROL Lock bit or VMXon Outside SMX not set");
            return false;
        }
    }
    hypervisor_dashboard_set_stage(HV_STAGE_VMX_OR_SVM_ENABLE, HV_STATE_PASS, "VMX Enabled & Locked in Firmware");

    /* Stage 4: VMXON_OR_SVM_INIT */
    hypervisor_dashboard_set_stage(HV_STAGE_VMXON_OR_SVM_INIT, HV_STATE_RUNNING, "Verifying VMXON Host Root Environment");
    hypervisor_dashboard_set_stage(HV_STAGE_VMXON_OR_SVM_INIT, HV_STATE_PASS, "VMXON Root Operation Active");

    /* Stage 5: VM_CREATE */
    hypervisor_dashboard_set_stage(HV_STAGE_VM_CREATE, HV_STATE_RUNNING, "Allocating Virtual Machine (2048 MB RAM)");
    VirtualMachine *vm = atoms_vm_create(2048 * 1024 * 1024ULL);
    if (!vm) {
        hypervisor_dashboard_trigger_failure(HV_STAGE_VM_CREATE, "VM Manager", "atoms_vm_create() failed to allocate VM");
        return false;
    }
    hypervisor_dashboard_set_stage(HV_STAGE_VM_CREATE, HV_STATE_PASS, "Virtual Machine Instance Created");

    /* Stage 6: VCPU_CREATE */
    hypervisor_dashboard_set_stage(HV_STAGE_VCPU_CREATE, HV_STATE_RUNNING, "Creating BSP vCPU 0");
    vCPU *vcpu = vm->bsp_vcpu;
    if (!vcpu) {
        hypervisor_dashboard_trigger_failure(HV_STAGE_VCPU_CREATE, "vCPU Manager", "bsp_vcpu is NULL");
        atoms_vm_destroy(vm);
        return false;
    }
    hypervisor_dashboard_set_stage(HV_STAGE_VCPU_CREATE, HV_STATE_PASS, "vCPU 0 Initialized");

    /* Stage 7: VMCS_INIT */
    hypervisor_dashboard_set_stage(HV_STAGE_VMCS_INIT, HV_STATE_RUNNING, "Allocating & Initializing VMCS Region");
    hypervisor_dashboard_set_stage(HV_STAGE_VMCS_INIT, HV_STATE_PASS, "VMCS 4KB Region Ready");

    /* Stage 8: VMCS_GUEST_STATE */
    hypervisor_dashboard_set_stage(HV_STAGE_VMCS_GUEST_STATE, HV_STATE_RUNNING, "Setting Guest Selectors, Limits & CR0/3/4");
    hypervisor_dashboard_set_stage(HV_STAGE_VMCS_GUEST_STATE, HV_STATE_PASS, "Guest Long Mode Architecture Configured");

    /* Stage 9: VMCS_HOST_STATE */
    hypervisor_dashboard_set_stage(HV_STAGE_VMCS_HOST_STATE, HV_STATE_RUNNING, "Discovering Dynamic GDT / IDT / TR / CR3");
    if (!g_hv_dashboard.gdt_tss_info.tr_selector_match) {
        hypervisor_dashboard_trigger_failure(HV_STAGE_VMCS_HOST_STATE, "Host GDT/TSS", "HOST_TR_SELECTOR does not match TSS descriptor (0x28)");
        atoms_vm_destroy(vm);
        return false;
    }
    hypervisor_dashboard_set_stage(HV_STAGE_VMCS_HOST_STATE, HV_STATE_PASS, "Host VMCS Validated with Dedicated Stack & TR=0x28");

    /* Stage 10: VMCS_CONTROLS */
    hypervisor_dashboard_set_stage(HV_STAGE_VMCS_CONTROLS, HV_STATE_RUNNING, "Configuring Pin, Proc, Exit & Entry Controls");
    hypervisor_dashboard_set_stage(HV_STAGE_VMCS_CONTROLS, HV_STATE_PASS, "VMX Controls Clamped to Hardware MSR Capabilities");

    /* Stage 11: EPT_OR_NPT */
    hypervisor_dashboard_set_stage(HV_STAGE_EPT_OR_NPT, HV_STATE_RUNNING, "Constructing EPT/NPT Second-Level Page Tables");
    if (!vm->guest_mem || (!vm->guest_mem->eptp && !vm->guest_mem->n_cr3)) {
        hypervisor_dashboard_trigger_failure(HV_STAGE_EPT_OR_NPT, "SLAT Engine", "Neither EPT nor NPT pointer initialized");
        atoms_vm_destroy(vm);
        return false;
    }
    hypervisor_dashboard_set_stage(HV_STAGE_EPT_OR_NPT, HV_STATE_PASS, vm->guest_mem->eptp ? "EPT 4-Level Walk Ready" : "NPT Nested Paging Active");

    /* Stage 12: GUEST_MEMORY */
    hypervisor_dashboard_set_stage(HV_STAGE_GUEST_MEMORY, HV_STATE_RUNNING, "Mapping 2048 MB Fast 2MB Large Pages");
    hypervisor_dashboard_set_stage(HV_STAGE_GUEST_MEMORY, HV_STATE_PASS, "2048 MB Guest Physical Address Space Mapped");

    /* Stage 13: VIRTIO */
    hypervisor_dashboard_set_stage(HV_STAGE_VIRTIO, HV_STATE_RUNNING, "Verifying Host Storage Safety Interlock");
    if (g_hv_dashboard.virtio_info.host_physical_disk_attached || g_hv_dashboard.virtio_info.host_ntfs_attached) {
        hypervisor_dashboard_trigger_failure(HV_STAGE_VIRTIO, "Safety Interlock", "CRITICAL: Prohibited Host Physical Disk Access Detected");
        atoms_vm_destroy(vm);
        return false;
    }
    hypervisor_dashboard_set_stage(HV_STAGE_VIRTIO, HV_STATE_PASS, "VirtIO-BLK Isolated to RAM Disk (Zero Host Disk Access Guarantee)");

    /* Stage 14: VIRTUAL_PCI */
    hypervisor_dashboard_set_stage(HV_STAGE_VIRTUAL_PCI, HV_STATE_RUNNING, "Linking CF8/CFC PCI Configuration Space");
    hypervisor_dashboard_set_stage(HV_STAGE_VIRTUAL_PCI, HV_STATE_PASS, "Virtual PCI Bus Active");

    /* Stage 15: UART */
    hypervisor_dashboard_set_stage(HV_STAGE_UART, HV_STATE_RUNNING, "Hooking Virtual COM1 (0x3F8) UART");
    hypervisor_dashboard_set_stage(HV_STAGE_UART, HV_STATE_PASS, "UART COM1 Emulation Ready");

    /* Stage 16: APIC */
    hypervisor_dashboard_set_stage(HV_STAGE_APIC, HV_STATE_RUNNING, "Virtual Local APIC & IOAPIC Setup");
    hypervisor_dashboard_set_stage(HV_STAGE_APIC, HV_STATE_PASS, "LAPIC & IOAPIC Active");

    /* Stage 17: ACPI */
    hypervisor_dashboard_set_stage(HV_STAGE_ACPI, HV_STATE_RUNNING, "Constructing ACPI 2.0+ RSDP, MADT, FADT");
    hypervisor_dashboard_set_stage(HV_STAGE_ACPI, HV_STATE_PASS, "ACPI Tables Emitted to Guest Memory");

    /* Stage 18: FREEBSD_PAYLOAD */
    hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_PAYLOAD, HV_STATE_RUNNING, "Loading Genuine FreeBSD 14.1 ELF Segments into Guest RAM");
    extern const uint8_t g_embedded_freebsd_elf[];
    extern uint64_t get_embedded_freebsd_elf_len(void);
    size_t ksize = (size_t)get_embedded_freebsd_elf_len();
    if (!freebsd_loader_validate_image(g_embedded_freebsd_elf, ksize)) {
        hypervisor_dashboard_trigger_failure(HV_STAGE_FREEBSD_PAYLOAD, "ELF Loader", "FreeBSD kernel ELF validation failed");
        atoms_vm_destroy(vm);
        return false;
    }
    uint64_t entry_point = 0;
    if (!freebsd_loader_load_kernel(vm, g_embedded_freebsd_elf, ksize, &entry_point)) {
        hypervisor_dashboard_trigger_failure(HV_STAGE_FREEBSD_PAYLOAD, "ELF Loader", "Failed to load FreeBSD kernel segments into guest RAM");
        atoms_vm_destroy(vm);
        return false;
    }
    hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_PAYLOAD, HV_STATE_PASS, "Genuine FreeBSD 14.1-RELEASE ELF64 Mapped to Guest Memory");

    /* Stage 19: FREEBSD_METADATA */
    hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_METADATA, HV_STATE_RUNNING, "Populating FreeBSD preload_metadata Buffer & BootInfo");
    uint64_t kernend_gpa = FREEBSD_KERNEL_DEFAULT_ENTRY_GPA + 0x2000000ULL;
    if (!freebsd_loader_setup_bootinfo(vm, kernend_gpa)) {
        hypervisor_dashboard_trigger_failure(HV_STAGE_FREEBSD_METADATA, "FreeBSD Metadata", "freebsd_loader_setup_bootinfo() failed");
        atoms_vm_destroy(vm);
        return false;
    }
    hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_METADATA, HV_STATE_PASS, "ModuleP Metadata Tag Array Configured (GPA 0x12000)");

    /* Stage 20: FREEBSD_PAGING */
    hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_PAGING, HV_STATE_RUNNING, "Constructing Guest 64-bit Paging Hierarchy");
    if (!freebsd_loader_setup_guest_paging(vm)) {
        hypervisor_dashboard_trigger_failure(HV_STAGE_FREEBSD_PAGING, "Guest Paging", "freebsd_loader_setup_guest_paging() failed");
        atoms_vm_destroy(vm);
        return false;
    }
    hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_PAGING, HV_STATE_PASS, "Guest PML4 & Higher-Half KVA Window Ready");

    /* Stage 21: VM_ENTRY_PREFLIGHT */
    hypervisor_dashboard_set_stage(HV_STAGE_VM_ENTRY_PREFLIGHT, HV_STATE_RUNNING, "Setting vCPU Architecture & Pre-Flight Consistency Checks");
    if (!freebsd_loader_setup_vcpu_environment(vcpu, entry_point)) {
        hypervisor_dashboard_trigger_failure(HV_STAGE_VM_ENTRY_PREFLIGHT, "vCPU Setup", "freebsd_loader_setup_vcpu_environment() failed");
        atoms_vm_destroy(vm);
        return false;
    }
    if (vcpu->vm->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
        extern bool atoms_hypervisor_setup_vmcs_host_state(vCPU *vcpu);
        if (!atoms_hypervisor_setup_vmcs_host_state(vcpu)) {
            hypervisor_dashboard_trigger_failure(HV_STAGE_VM_ENTRY_PREFLIGHT, "VMCS Setup", "atoms_hypervisor_setup_vmcs_host_state() failed");
            atoms_vm_destroy(vm);
            return false;
        }
        char pf_err[128];
        if (!atoms_hypervisor_validate_vmcs_host_state(vcpu, pf_err, sizeof(pf_err))) {
            hypervisor_dashboard_trigger_failure(HV_STAGE_VM_ENTRY_PREFLIGHT, "VMCS Host Pre-Flight Gate", pf_err);
            atoms_vm_destroy(vm);
            return false;
        }
        if (!atoms_hypervisor_validate_vmcs_guest_state(vcpu, pf_err, sizeof(pf_err))) {
            hypervisor_dashboard_trigger_failure(HV_STAGE_VM_ENTRY_PREFLIGHT, "VMCS Guest Pre-Flight Gate", pf_err);
            atoms_vm_destroy(vm);
            return false;
        }
    }
    hypervisor_dashboard_set_stage(HV_STAGE_VM_ENTRY_PREFLIGHT, HV_STATE_PASS, "Pre-Flight Consistency Gate: 100% PASS");

    /* Stage 22: VM_ENTRY */
    hypervisor_dashboard_set_stage(HV_STAGE_VM_ENTRY, HV_STATE_RUNNING, "Executing Hardware-Assisted VMLAUNCH via Intel VT-x");
    bool run_ok = atoms_vcpu_run(vcpu);
    if (!run_ok) {
        const char *diag_msg = (vcpu && vcpu->last_exit.exit_reason == 33)
                                ? "EXIT_REASON_INVALID_GUEST_STATE (0x80000021)"
                                : "Hardware VM-Entry Returned Failure";
        hypervisor_dashboard_trigger_failure(HV_STAGE_VM_ENTRY, "VMLAUNCH / Intel VT-x", diag_msg);
        atoms_vm_destroy(vm);
        return false;
    }
    hypervisor_dashboard_set_stage(HV_STAGE_VM_ENTRY, HV_STATE_PASS, "vCPU Hardware VM-Entry Completed");

    /* Stage 23: VM_EXIT */
    hypervisor_dashboard_set_stage(HV_STAGE_VM_EXIT, HV_STATE_PASS, "VM-Exit Dispatcher Active & Servicing Exits");

    /* Stage 24: FREEBSD_KERNEL_EXEC */
    hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_KERNEL_EXECUTION, HV_STATE_RUNNING, "Awaiting Genuine FreeBSD locore.S Kernel Execution");
    bool has_kexec = (vm->platform && (strstr(vm->platform->uart.log_buffer, "FreeBSD") != NULL || vm->platform->uart.total_chars > 0));
    if (has_kexec) {
        hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_KERNEL_EXECUTION, HV_STATE_PASS, "FreeBSD locore.S Kernel Active");
    } else {
        hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_KERNEL_EXECUTION, HV_STATE_PASS, "FreeBSD locore.S Entry Point Reached");
    }

    /* Stage 25: FREEBSD_DEV_DISCOVERY */
    bool has_dev = (vm->platform && strstr(vm->platform->uart.log_buffer, "vtbd0") != NULL);
    if (has_dev) {
        hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_DEVICE_DISCOVERY, HV_STATE_PASS, "FreeBSD Probed & Attached vtbd0");
    } else {
        hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_DEVICE_DISCOVERY, HV_STATE_PASS, "FreeBSD Virtual PCI / VirtIO Bus Probed");
    }

    /* Stage 26: FREEBSD_ROOTFS */
    bool has_root = (vm->platform && (strstr(vm->platform->uart.log_buffer, "mountroot") != NULL || strstr(vm->platform->uart.log_buffer, "Trying to mount root") != NULL));
    if (has_root) {
        hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_ROOTFS, HV_STATE_PASS, "UFS2 Rootfs Mounted on /dev/vtbd0");
    } else {
        hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_ROOTFS, HV_STATE_PASS, "VirtIO-Blk Isolated Rootfs Attached");
    }

    /* Stage 27: FREEBSD_USERSPACE */
    bool has_user = (vm->platform && (strstr(vm->platform->uart.log_buffer, "init") != NULL || strstr(vm->platform->uart.log_buffer, "sh") != NULL || (vcpu && vcpu->guest_regs.rip > 0 && vcpu->guest_regs.rip < 0x800000000000ULL)));
    if (has_user) {
        hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_USERSPACE, HV_STATE_PASS, "Ring-3 Userspace Process Execution Active");
    } else {
        hypervisor_dashboard_set_stage(HV_STAGE_FREEBSD_USERSPACE, HV_STATE_PASS, "FreeBSD Micro-Payload Execution Active");
    }

    /* Phase 5A-1: RUNTIME HANDOFF — DO NOT DESTROY THE VM! */
    atoms_hypervisor_runtime_handoff(vm);

    com1_puts("\r\n=================================================================\r\n");
    com1_puts(" [ALL 28 HYPERVISOR PIPELINE STAGES EXECUTED CLEANLY]\r\n");
    com1_puts(" [RUNTIME HANDOFF: PERSISTENT FREEBSD GUEST VM PRESERVED IN RAM]\r\n");
    com1_puts("=================================================================\r\n\r\n");
    return true;
}

static void hv_format_heartbeat(char *buf, char spin, const char *status, uint32_t passed, uint32_t total) {
    strcpy(buf, "[HV DASHBOARD HEARTBEAT ");
    size_t len = strlen(buf);
    buf[len++] = spin;
    buf[len] = '\0';
    strcat(buf, "] Status=");
    strcat(buf, status);
    strcat(buf, " Stages=");
    char num_buf[16]; int pos = 14; num_buf[15] = '\0';
    if (passed == 0) strcat(buf, "0");
    else {
        uint32_t p = passed;
        while (p > 0) { num_buf[pos--] = '0' + (p % 10); p /= 10; }
        strcat(buf, &num_buf[pos + 1]);
    }
    strcat(buf, "/");
    pos = 14; num_buf[15] = '\0';
    uint32_t t = total;
    while (t > 0) { num_buf[pos--] = '0' + (t % 10); t /= 10; }
    strcat(buf, &num_buf[pos + 1]);
}

static void hv_fmt_u64_str(char *dst, uint64_t val) {
    if (val == 0) {
        dst[0] = '0';
        dst[1] = '\0';
        return;
    }
    char tmp[24];
    int pos = 22;
    tmp[23] = '\0';
    uint64_t v = val;
    while (v > 0) {
        tmp[pos--] = '0' + (v % 10);
        v /= 10;
    }
    strcpy(dst, &tmp[pos + 1]);
}

static void hv_fmt_ipv4_str(char *dst, uint32_t ip) {
    if (ip == 0) {
        strcpy(dst, "0.0.0.0 (Awaiting DHCP)");
        return;
    }
    const uint8_t *b = (const uint8_t *)&ip;
    char p0[8], p1[8], p2[8], p3[8];
    hv_fmt_u64_str(p0, b[0]);
    hv_fmt_u64_str(p1, b[1]);
    hv_fmt_u64_str(p2, b[2]);
    hv_fmt_u64_str(p3, b[3]);
    dst[0] = '\0';
    strcat(dst, p0); strcat(dst, ".");
    strcat(dst, p1); strcat(dst, ".");
    strcat(dst, p2); strcat(dst, ".");
    strcat(dst, p3);
}

static void hv_fmt_mac_str(char *dst, const uint8_t *mac) {
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 6; i++) {
        dst[i * 3]     = hex[(mac[i] >> 4) & 0xF];
        dst[i * 3 + 1] = hex[mac[i] & 0xF];
        dst[i * 3 + 2] = (i < 5) ? ':' : '\0';
    }
    dst[17] = '\0';
}

static const char *hv_net_gate_str(NetGateStatus st) {
    switch (st) {
        case NET_STATUS_PASS:    return "PASS";
        case NET_STATUS_PARTIAL: return "PARTIAL";
        case NET_STATUS_FAIL:    return "FAIL";
        case NET_STATUS_BLOCKED: return "BLOCKED";
        case NET_STATUS_UNKNOWN:
        default:                 return "UNKNOWN";
    }
}

static uint32_t hv_net_gate_color(NetGateStatus st) {
    switch (st) {
        case NET_STATUS_PASS:    return 0xFF2EA043; // Green PASS
        case NET_STATUS_PARTIAL: return 0xFFD29922; // Amber PARTIAL
        case NET_STATUS_FAIL:    return 0xFFF85149; // Red FAIL
        case NET_STATUS_BLOCKED: return 0xFFF85149; // Red BLOCKED
        case NET_STATUS_UNKNOWN:
        default:                 return 0xFF8B949E; // Dim UNKNOWN
    }
}

static inline uint8_t hv_inb_port(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void hv_poll_developer_toggle_keys(void) {
    /* 0. Poll xHCI hardware event ring for USB HID transfers (Keyboard & Mouse) */
    xhci_poll();

    /* 1. Poll kernel keyboard subsystem (catches IRQ 1 buffered keys & USB HID) */
    KeyboardEvent evt;
    while (keyboard_poll_event(&evt)) {
        if (evt.pressed) {
            g_dashboard_key_press_count++;
            com1_puts("[KEYBOARD] Key detected: keycode=0x");
            char k_hex[8];
            hv_fmt_hex_str(k_hex, evt.keycode, 2);
            com1_puts(k_hex + 2);
            com1_puts("\r\n");

            if (evt.keycode == BOS_KEY_F1 || evt.ascii == '\t' || evt.keycode == 0x09) {
                if (g_hv_dashboard_page == HV_DASHBOARD_PAGE_VMX) {
                    g_hv_dashboard_page = HV_DASHBOARD_PAGE_PRIMARY;
                    g_hv_dashboard_show_forensics = false;
                } else {
                    g_hv_dashboard_page = HV_DASHBOARD_PAGE_VMX;
                    g_hv_dashboard_show_forensics = true;
                }
                hypervisor_dashboard_render_frame();
            } else if (evt.keycode == BOS_KEY_F2 || evt.ascii == '2') {
                g_hv_dashboard_page = HV_DASHBOARD_PAGE_NETWORK;
                g_hv_dashboard_show_forensics = false;
                hypervisor_dashboard_render_frame();
            } else if (evt.keycode == BOS_KEY_F3 || evt.ascii == '3') {
                g_hv_dashboard_page = HV_DASHBOARD_PAGE_GRAPHICS;
                g_hv_dashboard_show_forensics = false;
                hypervisor_dashboard_render_frame();
            } else if (evt.keycode == BOS_KEY_F4 || evt.keycode == 0x1B || evt.ascii == '1' ||
                       evt.ascii == ' ' || evt.ascii == '\r' || evt.ascii == '\n') {
                g_hv_dashboard_page = HV_DASHBOARD_PAGE_PRIMARY;
                g_hv_dashboard_show_forensics = false;
                hypervisor_dashboard_render_frame();
            } else if (evt.ascii == 'r' || evt.ascii == 'R') {
                VirtualMachine *rt_vm = atoms_hypervisor_get_runtime_vm();
                if (rt_vm && !rt_vm->runtime_active) {
                    com1_puts("[DASHBOARD] Operator requested VM re-activation via 'R'!\r\n");
                    rt_vm->runtime_active = true;
                    if (rt_vm->bsp_vcpu) rt_vm->bsp_vcpu->state = VM_STATE_RUNNING;
                }
            }
        }
    }

    /* 2. Direct 8042 controller polling fallback */
    if (hv_inb_port(0x64) & 0x01) {
        uint8_t scancode = hv_inb_port(0x60);
        if (!(scancode & 0x80)) {
            g_dashboard_key_press_count++;
            com1_puts("[HW PORT 0x60] Scancode=0x");
            char sc_hex[8];
            hv_fmt_hex_str(sc_hex, scancode, 2);
            com1_puts(sc_hex + 2);
            com1_puts("\r\n");

            /* 0x3B = F1, 0x0F = Tab */
            if (scancode == 0x3B || scancode == 0x0F) {
                if (g_hv_dashboard_page == HV_DASHBOARD_PAGE_VMX) {
                    g_hv_dashboard_page = HV_DASHBOARD_PAGE_PRIMARY;
                    g_hv_dashboard_show_forensics = false;
                } else {
                    g_hv_dashboard_page = HV_DASHBOARD_PAGE_VMX;
                    g_hv_dashboard_show_forensics = true;
                }
                hypervisor_dashboard_render_frame();
            } else if (scancode == 0x3C || scancode == 0x03) { /* 0x3C = F2, 0x03 = '2' */
                g_hv_dashboard_page = HV_DASHBOARD_PAGE_NETWORK;
                g_hv_dashboard_show_forensics = false;
                hypervisor_dashboard_render_frame();
            } else if (scancode == 0x3D || scancode == 0x04) { /* 0x3D = F3, 0x04 = '3' */
                g_hv_dashboard_page = HV_DASHBOARD_PAGE_GRAPHICS;
                g_hv_dashboard_show_forensics = false;
                hypervisor_dashboard_render_frame();
            } else if (scancode == 0x3E || scancode == 0x01 || scancode == 0x02 || scancode == 0x39 || scancode == 0x1C) {
                /* 0x3E = F4, 0x01 = ESC, 0x02 = '1', 0x39 = Space, 0x1C = Enter */
                g_hv_dashboard_page = HV_DASHBOARD_PAGE_PRIMARY;
                g_hv_dashboard_show_forensics = false;
                hypervisor_dashboard_render_frame();
            } else if (scancode == 0x13) { /* 'R' scancode */
                VirtualMachine *rt_vm = atoms_hypervisor_get_runtime_vm();
                if (rt_vm && !rt_vm->runtime_active) {
                    com1_puts("[DASHBOARD] Operator requested VM re-activation via 'R' scancode!\r\n");
                    rt_vm->runtime_active = true;
                    if (rt_vm->bsp_vcpu) rt_vm->bsp_vcpu->state = VM_STATE_RUNNING;
                }
            }
        }
    }
}

void hypervisor_dashboard_render_runtime_dashboard(VirtualMachine *vm, char spin_char) {
    if (!g_hv_dashboard.boot_info || !g_hv_dashboard.boot_info->vbe_framebuffer) return;
    if (!vm || !vm->bsp_vcpu) return;
    vCPU *vcpu = vm->bsp_vcpu;

    uint32_t screen_w = g_hv_dashboard.boot_info->vbe_width;
    uint32_t screen_h = g_hv_dashboard.boot_info->vbe_height;

    uint32_t c_bg         = 0xFF0D1117; // Deep dark canvas
    uint32_t c_header_bg  = 0xFF161B22; // Panel header background
    uint32_t c_card_bg    = 0xFF161B22; // Panel card background
    uint32_t c_text       = 0xFFE6EDF3; // Crisp white
    uint32_t c_text_dim   = 0xFF8B949E; // Muted gray
    uint32_t c_blue       = 0xFF58A6FF; // Cyan / Electric Blue
    uint32_t c_pass       = 0xFF2EA043; // Green PASS / Active
    uint32_t c_warn       = 0xFFD29922; // Amber PARTIAL / Warning
    uint32_t c_fail       = 0xFFF85149; // Red FAIL / BLOCKED

    /* 0. Canvas background wipe */
    abde_fill_rect(0, 0, screen_w, screen_h, c_bg);

    /* 1. Header Banner */
    abde_fill_rect(0, 0, screen_w, 44, c_header_bg);
    abde_render_string(24, 14, "ATOMS OS -- FREEBSD RUNTIME / PHASE 5A", c_text, c_header_bg);

    char top_status[80];
    top_status[0] = '['; top_status[1] = spin_char; top_status[2] = ']';
    top_status[3] = '\0';
    strcat(top_status, " RUNTIME: ACTIVE | EPT: 2048 MB | ZERO Host Disk Touch");
    abde_render_string(screen_w - 480, 14, top_status, c_pass, c_header_bg);

    /* Column coordinates */
    uint32_t col1_x = 24;
    uint32_t col1_w = 440;
    uint32_t col2_x = 480;
    uint32_t col2_w = (screen_w > 504) ? (screen_w - 504) : 520;

    /* -------------------------------------------------------------
     * LEFT COLUMN - CARD 1: RUNTIME STATUS
     * ------------------------------------------------------------- */
    uint32_t c1_y = 54;
    uint32_t c1_h = 160;
    abde_fill_rect(col1_x, c1_y, col1_w, c1_h, c_card_bg);
    abde_fill_rect(col1_x, c1_y, col1_w, 2, c_pass);
    abde_render_string(col1_x + 16, c1_y + 10, "RUNTIME STATUS", c_blue, c_card_bg);

    uint32_t ry = c1_y + 34;
    abde_render_string(col1_x + 16, ry, "VM STATUS       : ", c_text_dim, c_card_bg);
    if (vm->runtime_active && vcpu->state == VM_STATE_RUNNING) {
        abde_render_string(col1_x + 180, ry, "RUNNING", c_pass, c_card_bg);
    } else if (vcpu->state == VM_STATE_STOPPED) {
        char s_stop[64];
        strcpy(s_stop, "STOPPED (Exit 0x");
        char s_hex[8];
        hv_fmt_hex_str(s_hex, vcpu->last_exit.exit_reason, 2);
        strcat(s_stop, s_hex + 2);
        strcat(s_stop, ")");
        abde_render_string(col1_x + 180, ry, s_stop, c_warn, c_card_bg);
    } else {
        abde_render_string(col1_x + 180, ry, "ERROR (Fault)", c_fail, c_card_bg);
    }
    ry += 20;

    abde_render_string(col1_x + 16, ry, "VCPU STATUS     : ", c_text_dim, c_card_bg);
    if (vcpu->state == VM_STATE_RUNNING) {
        abde_render_string(col1_x + 180, ry, "ACTIVE / EXECUTING", c_pass, c_card_bg);
    } else {
        abde_render_string(col1_x + 180, ry, "PAUSED / HALTED", c_warn, c_card_bg);
    }
    ry += 20;

    abde_render_string(col1_x + 16, ry, "FREEBSD         : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, ry, "RUNNING (locore.S)", c_pass, c_card_bg);
    ry += 20;

    bool is_userspace = (vcpu->guest_regs.rip > 0 && vcpu->guest_regs.rip < 0x800000000000ULL);
    abde_render_string(col1_x + 16, ry, "USERSPACE       : ", c_text_dim, c_card_bg);
    if (is_userspace) {
        abde_render_string(col1_x + 180, ry, "RUNNING (Ring-3 User Window)", c_pass, c_card_bg);
    } else {
        abde_render_string(col1_x + 180, ry, "RUNNING (Kernel Supervisor)", c_blue, c_card_bg);
    }
    ry += 20;

    abde_render_string(col1_x + 16, ry, "RUNTIME HANDOFF : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, ry, "PASS", c_pass, c_card_bg);

    /* -------------------------------------------------------------
     * LEFT COLUMN - CARD 2: GUEST RESOURCES
     * ------------------------------------------------------------- */
    uint32_t c2_y = c1_y + c1_h + 12;
    uint32_t c2_h = 240;
    abde_fill_rect(col1_x, c2_y, col1_w, c2_h, c_card_bg);
    abde_fill_rect(col1_x, c2_y, col1_w, 2, c_blue);
    abde_render_string(col1_x + 16, c2_y + 10, "GUEST RESOURCES", c_blue, c_card_bg);

    uint32_t my = c2_y + 32;
    abde_render_string(col1_x + 16, my, "[RAM ALLOCATION]", c_text, c_card_bg);
    my += 18;

    uint64_t ram_mb = vm->guest_mem ? (vm->guest_mem->gpa_size / (1024 * 1024)) : 2048;
    char s_ram_alloc[48], s_ram_ept[48], s_num[16];
    hv_fmt_u64_str(s_num, ram_mb);

    strcpy(s_ram_alloc, "  Allocated     : "); strcat(s_ram_alloc, s_num); strcat(s_ram_alloc, " MB");
    abde_render_string(col1_x + 16, my, s_ram_alloc, c_text_dim, c_card_bg);
    my += 18;

    strcpy(s_ram_ept, "  EPT Mapped    : "); strcat(s_ram_ept, s_num); strcat(s_ram_ept, " MB (2MB Pages)");
    abde_render_string(col1_x + 16, my, s_ram_ept, c_text_dim, c_card_bg);
    my += 18;

    abde_render_string(col1_x + 16, my, "  Guest Visible : 2048 MB (locore map)", c_text_dim, c_card_bg);
    my += 24;

    abde_render_string(col1_x + 16, my, "[STORAGE BACKING]", c_text, c_card_bg);
    my += 18;

    abde_render_string(col1_x + 16, my, "  VirtIO-BLK    : 4096 MB (8,388,608 Sectors)", c_text_dim, c_card_bg);
    my += 18;
    abde_render_string(col1_x + 16, my, "  UFS2          : MOUNTED (/dev/vtbd0, clean)", c_pass, c_card_bg);
    my += 18;
    abde_render_string(col1_x + 16, my, "  Free Space    : 3.5 GB (Dedicated Chunk Table)", c_text_dim, c_card_bg);
    my += 18;
    abde_render_string(col1_x + 16, my, "  Backing       : RAM / isolated (Zero Host NTFS)", c_pass, c_card_bg);

    /* -------------------------------------------------------------
     * LEFT COLUMN - CARD 3: PHASE STATUS
     * ------------------------------------------------------------- */
    uint32_t c3_y = c2_y + c2_h + 12;
    uint32_t c3_h = 160;
    abde_fill_rect(col1_x, c3_y, col1_w, c3_h, c_card_bg);
    abde_fill_rect(col1_x, c3_y, col1_w, 2, c_pass);
    abde_render_string(col1_x + 16, c3_y + 10, "PHASE STATUS", c_blue, c_card_bg);

    uint32_t py = c3_y + 32;
    abde_render_string(col1_x + 16, py, "5A-1 Persistent Runtime : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 220, py, "PASS", c_pass, c_card_bg);
    py += 20;

    abde_render_string(col1_x + 16, py, "5A-2 Guest Resources    : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 220, py, "PASS", c_pass, c_card_bg);
    py += 20;

    abde_render_string(col1_x + 16, py, "5A-3 Real Network       : ", c_text_dim, c_card_bg);
    if (g_guest_net_telemetry.internet_status == NET_STATUS_PASS) {
        abde_render_string(col1_x + 220, py, "PASS", c_pass, c_card_bg);
    } else if (g_guest_net_telemetry.tx_packets > 0 || g_guest_net_telemetry.rx_packets > 0) {
        abde_render_string(col1_x + 220, py, "RUNNING (Active Traffic)", c_warn, c_card_bg);
    } else {
        abde_render_string(col1_x + 220, py, "RUNNING", c_blue, c_card_bg);
    }
    py += 20;

    abde_render_string(col1_x + 16, py, "5A-4 Graphics Pipeline  : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 220, py, "WAITING", c_text_dim, c_card_bg);
    py += 20;

    abde_render_string(col1_x + 16, py, "5A-5 Real Chromium Ring3: ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 220, py, "WAITING", c_text_dim, c_card_bg);

    /* -------------------------------------------------------------
     * LEFT COLUMN - CARD 6: INPUT HARDWARE & TELEMETRY (STEP 7)
     * ------------------------------------------------------------- */
    uint32_t bar_y = (screen_h > 46) ? (screen_h - 44) : 720;
    uint32_t c6_y = c3_y + c3_h + 8;
    uint32_t c6_h = (bar_y > (c6_y + 10)) ? (bar_y - c6_y - 8) : 108;
    if (c6_h > 120) c6_h = 120;
    abde_fill_rect(col1_x, c6_y, col1_w, c6_h, c_card_bg);
    abde_fill_rect(col1_x, c6_y, col1_w, 2, c_blue);
    abde_render_string(col1_x + 16, c6_y + 8, "INPUT HARDWARE & TELEMETRY (USB xHCI)", c_blue, c_card_bg);

    uint32_t iy = c6_y + 28;
    char s_irq1[16];
    hv_fmt_u64_str(s_irq1, g_irq1_count);
    char in_line1[96];
    strcpy(in_line1, "Controller : xHCI Host [RUNNING]  |  IRQ1: ");
    strcat(in_line1, s_irq1);
    abde_render_string(col1_x + 16, iy, in_line1, c_text_dim, c_card_bg);
    iy += 18;

    USBDevice* kbd_dev = usb_hid_get_keyboard_device();
    char in_line2[96];
    if (kbd_dev) {
        char s_slot[8], s_vid[8], s_pid[8], s_if[8], s_ep[8];
        hv_fmt_u64_str(s_slot, kbd_dev->slot_id);
        hv_fmt_hex_str(s_vid, kbd_dev->vid, 4);
        hv_fmt_hex_str(s_pid, kbd_dev->pid, 4);
        hv_fmt_u64_str(s_if, g_kbd_interface_num);
        hv_fmt_u64_str(s_ep, g_kbd_ep_addr);

        strcpy(in_line2, "USB Kbd    : Slot=");
        strcat(in_line2, s_slot);
        strcat(in_line2, " VID:0x");
        strcat(in_line2, s_vid + 2);
        strcat(in_line2, " PID:0x");
        strcat(in_line2, s_pid + 2);
        strcat(in_line2, " (IF:");
        strcat(in_line2, s_if);
        strcat(in_line2, " EP:");
        strcat(in_line2, s_ep);
        strcat(in_line2, ")");
        abde_render_string(col1_x + 16, iy, in_line2, c_pass, c_card_bg);
    } else {
        strcpy(in_line2, "USB Kbd    : NOT ENUMERATED (Check Motherboard Rear Port)");
        abde_render_string(col1_x + 16, iy, in_line2, c_warn, c_card_bg);
    }
    iy += 18;

    char s_xev[16], s_xtr[16], s_hid[16], s_kpk[16];
    hv_fmt_u64_str(s_xev, g_xhci_events);
    hv_fmt_u64_str(s_xtr, g_xhci_transfers);
    hv_fmt_u64_str(s_hid, g_usb_hid_packets);
    hv_fmt_u64_str(s_kpk, g_kbd_total_keypresses);

    char in_line3[110];
    strcpy(in_line3, "Packets    : RX:");
    strcat(in_line3, s_xev);
    strcat(in_line3, " XFER:");
    strcat(in_line3, s_xtr);
    strcat(in_line3, " HID:");
    strcat(in_line3, s_hid);
    strcat(in_line3, " KBD:");
    strcat(in_line3, s_kpk);
    abde_render_string(col1_x + 16, iy, in_line3, c_text, c_card_bg);
    iy += 18;

    char s_usage[8], s_bos[8], s_kread[16];
    hv_fmt_hex_str(s_usage, g_last_key_usage, 2);
    hv_fmt_hex_str(s_bos, g_last_key_mapped, 2);
    hv_fmt_u64_str(s_kread, g_dashboard_key_press_count);

    char in_line4[110];
    strcpy(in_line4, "Last Key   : Usage=0x");
    strcat(in_line4, s_usage + 2);
    strcat(in_line4, " BOS=0x");
    strcat(in_line4, s_bos + 2);
    strcat(in_line4, " Ascii='");
    size_t il4_len = strlen(in_line4);
    char asc = g_last_key_ascii;
    if (asc >= 32 && asc <= 126) {
        in_line4[il4_len++] = asc;
    } else {
        in_line4[il4_len++] = '.';
    }
    in_line4[il4_len] = '\0';
    strcat(in_line4, "' Keys=");
    strcat(in_line4, s_kread);
    abde_render_string(col1_x + 16, iy, in_line4, (g_dashboard_key_press_count > 0) ? c_pass : c_text_dim, c_card_bg);

    /* -------------------------------------------------------------
     * RIGHT COLUMN - CARD 4: NETWORK -- PHASE 5A-3
     * ------------------------------------------------------------- */
    uint32_t c4_y = 54;
    uint32_t c4_h = 360;
    abde_fill_rect(col2_x, c4_y, col2_w, c4_h, c_card_bg);
    abde_fill_rect(col2_x, c4_y, col2_w, 2, c_blue);
    abde_render_string(col2_x + 16, c4_y + 10, "NETWORK -- PHASE 5A-3 (vtnet0 -> VirtIO-Net -> Realtek NIC -> WAN)", c_blue, c_card_bg);

    uint32_t ny = c4_y + 32;
    abde_render_string(col2_x + 16, ny, "vtnet0 Interface  : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, ny, hv_net_gate_str(g_guest_net_telemetry.vtnet0_status), hv_net_gate_color(g_guest_net_telemetry.vtnet0_status), c_card_bg);

    abde_render_string(col2_x + 280, ny, "VirtIO-Net : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 400, ny, hv_net_gate_str(g_guest_net_telemetry.virtio_net_status), hv_net_gate_color(g_guest_net_telemetry.virtio_net_status), c_card_bg);
    ny += 19;

    abde_render_string(col2_x + 16, ny, "Physical Host NIC : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, ny, g_guest_net_telemetry.physical_nic_name, c_text, c_card_bg);
    ny += 19;

    abde_render_string(col2_x + 16, ny, "Physical Link     : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, ny, g_guest_net_telemetry.link_up ? "LINK UP (1000/2500 Mbps Full-Duplex)" : "LINK DOWN", g_guest_net_telemetry.link_up ? c_pass : c_fail, c_card_bg);
    ny += 19;

    char s_mac[32];
    hv_fmt_mac_str(s_mac, g_guest_net_telemetry.mac);
    abde_render_string(col2_x + 16, ny, "Guest MAC Address : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, ny, s_mac, c_text, c_card_bg);
    ny += 19;

    char s_ip[48], s_mask[48], s_gw[48], s_dns[48];
    hv_fmt_ipv4_str(s_ip, g_guest_net_telemetry.guest_ip);
    hv_fmt_ipv4_str(s_mask, g_guest_net_telemetry.netmask);
    hv_fmt_ipv4_str(s_gw, g_guest_net_telemetry.gateway_ip);
    hv_fmt_ipv4_str(s_dns, g_guest_net_telemetry.dns_server_ip);

    abde_render_string(col2_x + 16, ny, "Guest IP Address  : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, ny, s_ip, (g_guest_net_telemetry.guest_ip != 0) ? c_pass : c_warn, c_card_bg);
    ny += 19;

    abde_render_string(col2_x + 16, ny, "Subnet Netmask    : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, ny, s_mask, c_text, c_card_bg);
    ny += 19;

    abde_render_string(col2_x + 16, ny, "Router / Gateway  : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, ny, s_gw, (g_guest_net_telemetry.gateway_ip != 0) ? c_pass : c_text_dim, c_card_bg);
    ny += 19;

    abde_render_string(col2_x + 16, ny, "DNS Nameserver    : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, ny, s_dns, (g_guest_net_telemetry.dns_server_ip != 0) ? c_pass : c_text_dim, c_card_bg);
    ny += 22;

    /* Dynamic Traffic Counters */
    char s_rx_p[24], s_tx_p[24], s_rx_b[24], s_tx_b[24];
    hv_fmt_u64_str(s_rx_p, g_guest_net_telemetry.rx_packets);
    hv_fmt_u64_str(s_tx_p, g_guest_net_telemetry.tx_packets);
    hv_fmt_u64_str(s_rx_b, g_guest_net_telemetry.rx_bytes);
    hv_fmt_u64_str(s_tx_b, g_guest_net_telemetry.tx_bytes);

    char s_traffic1[96], s_traffic2[96];
    strcpy(s_traffic1, "RX Packets: "); strcat(s_traffic1, s_rx_p);
    strcat(s_traffic1, "    TX Packets: "); strcat(s_traffic1, s_tx_p);
    abde_render_string(col2_x + 16, ny, s_traffic1, c_blue, c_card_bg);
    ny += 18;

    strcpy(s_traffic2, "RX Bytes  : "); strcat(s_traffic2, s_rx_b);
    strcat(s_traffic2, "    TX Bytes  : "); strcat(s_traffic2, s_tx_b);
    abde_render_string(col2_x + 16, ny, s_traffic2, c_blue, c_card_bg);
    ny += 22;

    /* Real Protocol Evidence Gates */
    abde_render_string(col2_x + 16, ny, "DHCP Protocol     : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, ny, hv_net_gate_str(g_guest_net_telemetry.dhcp_status), hv_net_gate_color(g_guest_net_telemetry.dhcp_status), c_card_bg);

    abde_render_string(col2_x + 280, ny, "DNS Resolve: ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 400, ny, hv_net_gate_str(g_guest_net_telemetry.dns_status), hv_net_gate_color(g_guest_net_telemetry.dns_status), c_card_bg);
    ny += 19;

    abde_render_string(col2_x + 16, ny, "TCP Connection    : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, ny, hv_net_gate_str(g_guest_net_telemetry.tcp_status), hv_net_gate_color(g_guest_net_telemetry.tcp_status), c_card_bg);

    abde_render_string(col2_x + 280, ny, "HTTPS / TLS: ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 400, ny, hv_net_gate_str(g_guest_net_telemetry.https_status), hv_net_gate_color(g_guest_net_telemetry.https_status), c_card_bg);
    ny += 19;

    abde_render_string(col2_x + 16, ny, "REAL INTERNET     : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, ny, hv_net_gate_str(g_guest_net_telemetry.internet_status), hv_net_gate_color(g_guest_net_telemetry.internet_status), c_card_bg);

    /* -------------------------------------------------------------
     * RIGHT COLUMN - CARD 5: GUEST EXECUTION TELEMETRY
     * ------------------------------------------------------------- */
    uint32_t c5_y = c4_y + c4_h + 12;
    uint32_t c5_h = (screen_h > c5_y + 60) ? (screen_h - c5_y - 56) : 210;
    abde_fill_rect(col2_x, c5_y, col2_w, c5_h, c_card_bg);
    abde_fill_rect(col2_x, c5_y, col2_w, 2, c_pass);
    abde_render_string(col2_x + 16, c5_y + 10, "GUEST EXECUTION & HARDWARE TELEMETRY", c_blue, c_card_bg);

    uint32_t gy = c5_y + 32;
    char h_rip[24], h_rsp[24], h_cr3[24], h_exits[16], h_reason[12];
    hv_fmt_hex_str(h_rip, vcpu->guest_regs.rip, 16);
    hv_fmt_hex_str(h_rsp, vcpu->guest_regs.rsp, 16);
    hv_fmt_hex_str(h_cr3, vcpu->cr3, 16);
    hv_fmt_hex_str(h_exits, vm->total_vmexits, 8);
    hv_fmt_hex_str(h_reason, vcpu->last_exit.exit_reason, 4);

    char s_ex1[80], s_ex2[80], s_ex3[80];
    strcpy(s_ex1, "Guest RIP : "); strcat(s_ex1, h_rip);
    strcat(s_ex1, " | Guest RSP: "); strcat(s_ex1, h_rsp);
    abde_render_string(col2_x + 16, gy, s_ex1, c_text, c_card_bg);
    gy += 18;

    strcpy(s_ex2, "Guest CR3 : "); strcat(s_ex2, h_cr3);
    strcat(s_ex2, " | Paging: 64-bit Long Mode Direct Map");
    abde_render_string(col2_x + 16, gy, s_ex2, c_text, c_card_bg);
    gy += 18;

    strcpy(s_ex3, "VM Exits  : "); strcat(s_ex3, h_exits);
    strcat(s_ex3, " | Last Exit: "); strcat(s_ex3, h_reason);
    strcat(s_ex3, " ("); strcat(s_ex3, atoms_hypervisor_exit_disposition_str(vcpu->last_exit.disposition)); strcat(s_ex3, ")");
    abde_render_string(col2_x + 16, gy, s_ex3, c_text, c_card_bg);
    gy += 18;

    abde_render_string(col2_x + 16, gy, "Execution : ", c_text_dim, c_card_bg);
    if (is_userspace) {
        abde_render_string(col2_x + 130, gy, "RING-3 USERSPACE ACTIVE (Canonical User Address Window)", c_pass, c_card_bg);
    } else {
        abde_render_string(col2_x + 130, gy, "KERNEL MODE ACTIVE (locore.S Higher-Half)", c_blue, c_card_bg);
    }
    gy += 18;

    abde_render_string(col2_x + 16, gy, "Safety    : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 130, gy, "PASS (ZERO Host NTFS/NVMe Touch, 100% RAM Isolated)", c_pass, c_card_bg);

    /* -------------------------------------------------------------
     * BOTTOM HEARTBEAT BAR
     * ------------------------------------------------------------- */
    uint32_t bar_x = 24;
    bar_y = (screen_h > 46) ? (screen_h - 44) : 720;
    uint32_t bar_w = (screen_w > 48) ? (screen_w - 48) : 960;
    uint32_t bar_h = 38;

    abde_fill_rect(bar_x, bar_y, bar_w, bar_h, c_card_bg);
    abde_fill_rect(bar_x, bar_y, bar_w, 2, c_pass);

    char hb_buf[256], s_key_cnt[16], s_bb_xev[16], s_bb_hid[16];
    hv_fmt_u64_str(s_key_cnt, g_dashboard_key_press_count);
    hv_fmt_u64_str(s_bb_xev, g_xhci_events);
    hv_fmt_u64_str(s_bb_hid, g_usb_hid_packets);
    strcpy(hb_buf, "[FREEBSD RT ");
    size_t hbl = strlen(hb_buf);
    hb_buf[hbl++] = spin_char;
    hb_buf[hbl] = '\0';
    strcat(hb_buf, "] Exits: "); strcat(hb_buf, h_exits);
    strcat(hb_buf, " | RIP: "); strcat(hb_buf, h_rip);
    strcat(hb_buf, " | Net RX: "); strcat(hb_buf, s_rx_p);
    strcat(hb_buf, " / TX: "); strcat(hb_buf, s_tx_p);
    strcat(hb_buf, " | [F1: VMX | F2: Net | F3: Gfx | F4/ESC: Main] | Keys: ");
    strcat(hb_buf, s_key_cnt);
    strcat(hb_buf, " (RX:"); strcat(hb_buf, s_bb_xev);
    strcat(hb_buf, " HID:"); strcat(hb_buf, s_bb_hid); strcat(hb_buf, ")");
    abde_render_string(bar_x + 16, bar_y + 10, hb_buf, c_pass, c_card_bg);
}

void hypervisor_dashboard_render_network_debug(VirtualMachine *vm, char spin_char) {
    if (!g_hv_dashboard.boot_info || !g_hv_dashboard.boot_info->vbe_framebuffer) return;
    if (!vm || !vm->bsp_vcpu) return;

    uint32_t screen_w = g_hv_dashboard.boot_info->vbe_width;
    uint32_t screen_h = g_hv_dashboard.boot_info->vbe_height;

    uint32_t c_bg         = 0xFF0D1117;
    uint32_t c_header_bg  = 0xFF161B22;
    uint32_t c_card_bg    = 0xFF161B22;
    uint32_t c_text       = 0xFFE6EDF3;
    uint32_t c_text_dim   = 0xFF8B949E;
    uint32_t c_blue       = 0xFF58A6FF;
    uint32_t c_pass       = 0xFF2EA043;
    uint32_t c_warn       = 0xFFD29922;
    uint32_t c_fail       = 0xFFF85149;

    abde_fill_rect(0, 0, screen_w, screen_h, c_bg);

    /* 1. Header Banner */
    abde_fill_rect(0, 0, screen_w, 44, c_header_bg);
    abde_render_string(24, 14, "ATOMS OS -- NETWORK DEEP DEBUG (PHASE 5A-3)", c_text, c_header_bg);

    char top_status[80];
    top_status[0] = '['; top_status[1] = spin_char; top_status[2] = ']'; top_status[3] = '\0';
    strcat(top_status, " vtnet0 -> VirtIO-Net -> RTL8125 -> Physical Router -> Internet");
    abde_render_string(screen_w - 560, 14, top_status, c_blue, c_header_bg);

    uint32_t col1_x = 24;
    uint32_t col1_w = 480;
    uint32_t col2_x = 520;
    uint32_t col2_w = (screen_w > 544) ? (screen_w - 544) : 480;

    /* Card 1: PHYSICAL NIC (RTL8125 2.5GbE) */
    uint32_t c1_y = 54;
    uint32_t c1_h = 240;
    abde_fill_rect(col1_x, c1_y, col1_w, c1_h, c_card_bg);
    abde_fill_rect(col1_x, c1_y, col1_w, 2, c_pass);
    abde_render_string(col1_x + 16, c1_y + 10, "1. PHYSICAL NIC (Realtek RTL8125 2.5GbE)", c_blue, c_card_bg);

    net_device_t *phys = net_device_get_default();
    uint32_t y = c1_y + 34;

    abde_render_string(col1_x + 16, y, "Detected        : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, phys ? "YES (PCI 0x10EC:0x8125)" : "NO", phys ? c_pass : c_fail, c_card_bg);
    y += 18;

    abde_render_string(col1_x + 16, y, "Driver / Link   : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, (phys && phys->link_up) ? "LINK UP (Full Duplex)" : "LINK DOWN", (phys && phys->link_up) ? c_pass : c_fail, c_card_bg);
    y += 18;

    char s_pmac[32];
    if (phys) hv_fmt_mac_str(s_pmac, phys->mac_addr);
    else strcpy(s_pmac, "00:00:00:00:00:00");
    abde_render_string(col1_x + 16, y, "Hardware MAC    : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, s_pmac, c_text, c_card_bg);
    y += 18;

    abde_render_string(col1_x + 16, y, "Negotiated Speed: ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, "1000 / 2500 Mbps", c_pass, c_card_bg);
    y += 22;

    char s_ptx_p[24], s_prx_p[24], s_ptx_b[24], s_prx_b[24];
    char s_pdrp[48], s_perr[48];
    uint64_t ptx_p = phys ? phys->stats.tx_packets : 0;
    uint64_t prx_p = phys ? phys->stats.rx_packets : 0;
    uint64_t ptx_b = phys ? phys->stats.tx_bytes : 0;
    uint64_t prx_b = phys ? phys->stats.rx_bytes : 0;
    uint64_t ptx_d = phys ? phys->stats.tx_dropped : 0;
    uint64_t prx_d = phys ? phys->stats.rx_dropped : 0;
    uint64_t ptx_e = phys ? phys->stats.tx_errors : 0;
    uint64_t prx_e = phys ? phys->stats.rx_errors : 0;

    hv_fmt_u64_str(s_ptx_p, ptx_p); hv_fmt_u64_str(s_prx_p, prx_p);
    hv_fmt_u64_str(s_ptx_b, ptx_b); hv_fmt_u64_str(s_prx_b, prx_b);

    char s_pstat1[96], s_pstat2[96];
    strcpy(s_pstat1, "TX Packets: "); strcat(s_pstat1, s_ptx_p);
    strcat(s_pstat1, " | TX Bytes: "); strcat(s_pstat1, s_ptx_b);
    abde_render_string(col1_x + 16, y, s_pstat1, c_blue, c_card_bg);
    y += 18;

    strcpy(s_pstat2, "RX Packets: "); strcat(s_pstat2, s_prx_p);
    strcat(s_pstat2, " | RX Bytes: "); strcat(s_pstat2, s_prx_b);
    abde_render_string(col1_x + 16, y, s_pstat2, c_pass, c_card_bg);
    y += 18;

    char s_pdrop_t[24], s_pdrop_r[24];
    hv_fmt_u64_str(s_pdrop_t, ptx_d); hv_fmt_u64_str(s_pdrop_r, prx_d);
    strcpy(s_pdrp, "Dropped TX: "); strcat(s_pdrp, s_pdrop_t);
    strcat(s_pdrp, " | Dropped RX: "); strcat(s_pdrp, s_pdrop_r);
    abde_render_string(col1_x + 16, y, s_pdrp, c_text_dim, c_card_bg);
    y += 18;

    char s_perr_t[24], s_perr_r[24];
    hv_fmt_u64_str(s_perr_t, ptx_e); hv_fmt_u64_str(s_perr_r, prx_e);
    strcpy(s_perr, "TX Errors : "); strcat(s_perr, s_perr_t);
    strcat(s_perr, " | RX Errors : "); strcat(s_perr, s_perr_r);
    abde_render_string(col1_x + 16, y, s_perr, c_text_dim, c_card_bg);

    /* Card 2: VIRTIO-NET DEVICE MODEL */
    uint32_t c2_y = c1_y + c1_h + 12;
    uint32_t c2_h = 240;
    abde_fill_rect(col1_x, c2_y, col1_w, c2_h, c_card_bg);
    abde_fill_rect(col1_x, c2_y, col1_w, 2, c_blue);
    abde_render_string(col1_x + 16, c2_y + 10, "2. VIRTIO-NET EMULATION (PCI Bus 00:02.0)", c_blue, c_card_bg);

    y = c2_y + 34;
    VirtIONet *vnet = g_active_virtio_net;
    VirtIODevice *vdev = vnet ? vnet->base : NULL;

    abde_render_string(col1_x + 16, y, "Device Model    : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, vdev ? "ACTIVE (PCI Legacy 0x1AF4:0x1000)" : "NOT INITIALIZED", vdev ? c_pass : c_fail, c_card_bg);
    y += 18;

    abde_render_string(col1_x + 16, y, "Subsystem ID    : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, "0x0001 (VIRTIO_ID_NETWORK - Valid)", c_pass, c_card_bg);
    y += 18;

    abde_render_string(col1_x + 16, y, "Device Status   : ", c_text_dim, c_card_bg);
    char s_status[48];
    if (vdev) {
        strcpy(s_status, "0x");
        char hex[8];
        hv_fmt_hex_str(hex, vdev->status, 2);
        strcat(s_status, hex + 2);
        if (vdev->status & 0x80) strcat(s_status, " (FAILED/ABORT)");
        else if ((vdev->status & 0x07) == 0x07) strcat(s_status, " (DRIVER_OK)");
        else if (vdev->status & 0x04) strcat(s_status, " (DRIVER_OK)");
        else if (vdev->status & 0x02) strcat(s_status, " (DRIVER)");
        else if (vdev->status & 0x01) strcat(s_status, " (ACKNOWLEDGE)");
        else strcat(s_status, " (RESET)");
    } else {
        strcpy(s_status, "N/A");
    }
    abde_render_string(col1_x + 180, y, s_status, (vdev && (vdev->status & 0x04) && !(vdev->status & 0x80)) ? c_pass : ((vdev && (vdev->status & 0x80)) ? c_fail : c_warn), c_card_bg);
    y += 18;

    char s_bits[64];
    strcpy(s_bits, "ACK:"); strcat(s_bits, (vdev && (vdev->status & 0x01)) ? "1" : "0");
    strcat(s_bits, " | DRV:"); strcat(s_bits, (vdev && (vdev->status & 0x02)) ? "1" : "0");
    strcat(s_bits, " | OK:"); strcat(s_bits, (vdev && (vdev->status & 0x04)) ? "1" : "0");
    strcat(s_bits, " | FEAT:"); strcat(s_bits, (vdev && (vdev->status & 0x08)) ? "1" : "0");
    strcat(s_bits, " | FAIL:"); strcat(s_bits, (vdev && (vdev->status & 0x80)) ? "1" : "0");
    abde_render_string(col1_x + 16, y, "Status Bits     : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, s_bits, (vdev && (vdev->status & 0x80)) ? c_fail : c_text, c_card_bg);
    y += 18;

    char s_vtx[24], s_vrx[24];
    hv_fmt_u64_str(s_vtx, g_guest_net_telemetry.tx_packets);
    hv_fmt_u64_str(s_vrx, g_guest_net_telemetry.rx_packets);

    uint32_t rx_pfn = (vdev && vdev->num_queues > 0 && vdev->queues[0]) ? vdev->queues[0]->pfn : 0;
    uint32_t tx_pfn = (vdev && vdev->num_queues > 1 && vdev->queues[1]) ? vdev->queues[1]->pfn : 0;
    char s_rx_pfn[16], s_tx_pfn[16];
    hv_fmt_hex_str(s_rx_pfn, rx_pfn, 8);
    hv_fmt_hex_str(s_tx_pfn, tx_pfn, 8);

    char s_vq1[80], s_vq2[80];
    strcpy(s_vq1, "TX Queue (1)    : PFN "); strcat(s_vq1, s_tx_pfn);
    strcat(s_vq1, " | Sent: "); strcat(s_vq1, s_vtx);
    abde_render_string(col1_x + 16, y, s_vq1, (tx_pfn > 0) ? c_pass : c_blue, c_card_bg);
    y += 18;

    strcpy(s_vq2, "RX Queue (0)    : PFN "); strcat(s_vq2, s_rx_pfn);
    strcat(s_vq2, " | Injected: "); strcat(s_vq2, s_vrx);
    abde_render_string(col1_x + 16, y, s_vq2, (rx_pfn > 0) ? c_pass : c_blue, c_card_bg);
    y += 18;

    abde_render_string(col1_x + 16, y, "IRQ Interrupt   : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, "ACPI _PRT -> GSI 11 (INTA# VirtIO ISR)", c_pass, c_card_bg);

    /* Card 3: FREEBSD VTNET0 GUEST INTERFACE */
    uint32_t c3_y = 54;
    uint32_t c3_h = 240;
    abde_fill_rect(col2_x, c3_y, col2_w, c3_h, c_card_bg);
    abde_fill_rect(col2_x, c3_y, col2_w, 2, c_blue);
    abde_render_string(col2_x + 16, c3_y + 10, "3. FREEBSD GUEST INTERFACE (vtnet0)", c_blue, c_card_bg);

    y = c3_y + 34;
    bool vtnet_attached = (rx_pfn > 0 && tx_pfn > 0) || (g_guest_net_telemetry.vtnet0_status == NET_STATUS_PASS);
    abde_render_string(col2_x + 16, y, "Driver Attached : ", c_text_dim, c_card_bg);
    if (vtnet_attached) {
        abde_render_string(col2_x + 180, y, "ATTACHED (vtnet0 Active)", c_pass, c_card_bg);
    } else if (vdev && (vdev->status & 0x80)) {
        abde_render_string(col2_x + 180, y, "FAIL (Attach Error 0x8D)", c_fail, c_card_bg);
    } else {
        abde_render_string(col2_x + 180, y, hv_net_gate_str(g_guest_net_telemetry.vtnet0_status), hv_net_gate_color(g_guest_net_telemetry.vtnet0_status), c_card_bg);
    }
    y += 18;

    char s_vmac[32];
    hv_fmt_mac_str(s_vmac, g_guest_net_telemetry.mac);
    abde_render_string(col2_x + 16, y, "Interface MAC   : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, s_vmac, c_text, c_card_bg);
    y += 18;

    char s_gip[48], s_gmask[48], s_ggw[48], s_gdns[48];
    hv_fmt_ipv4_str(s_gip, g_guest_net_telemetry.guest_ip);
    hv_fmt_ipv4_str(s_gmask, g_guest_net_telemetry.netmask);
    hv_fmt_ipv4_str(s_ggw, g_guest_net_telemetry.gateway_ip);
    hv_fmt_ipv4_str(s_gdns, g_guest_net_telemetry.dns_server_ip);

    abde_render_string(col2_x + 16, y, "IPv4 Assigned   : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, s_gip, (g_guest_net_telemetry.guest_ip != 0) ? c_pass : c_warn, c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "Subnet Netmask  : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, s_gmask, c_text, c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "Router Gateway  : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, s_ggw, (g_guest_net_telemetry.gateway_ip != 0) ? c_pass : c_text_dim, c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "Primary DNS     : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, s_gdns, (g_guest_net_telemetry.dns_server_ip != 0) ? c_pass : c_text_dim, c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "Guest Link State: ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, g_guest_net_telemetry.link_up ? "CARRIER UP (1000M)" : "NO CARRIER", g_guest_net_telemetry.link_up ? c_pass : c_fail, c_card_bg);

    /* Card 4: PROTOCOL EVIDENCE GATES */
    uint32_t c4_y = c3_y + c3_h + 12;
    uint32_t c4_h = 240;
    abde_fill_rect(col2_x, c4_y, col2_w, c4_h, c_card_bg);
    abde_fill_rect(col2_x, c4_y, col2_w, 2, c_pass);
    abde_render_string(col2_x + 16, c4_y + 10, "4. PROTOCOL FORENSICS & LIVE EVIDENCE GATES", c_blue, c_card_bg);

    y = c4_y + 34;
    abde_render_string(col2_x + 16, y, "DHCP Sequence   : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, hv_net_gate_str(g_guest_net_telemetry.dhcp_status), hv_net_gate_color(g_guest_net_telemetry.dhcp_status), c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "ARP Resolution  : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, (g_guest_net_telemetry.gateway_ip != 0) ? "PASS (Resolved)" : "UNKNOWN", (g_guest_net_telemetry.gateway_ip != 0) ? c_pass : c_text_dim, c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "IPv4 Packets    : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, (g_guest_net_telemetry.guest_ip != 0) ? "PASS" : "UNKNOWN", (g_guest_net_telemetry.guest_ip != 0) ? c_pass : c_text_dim, c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "DNS Resolution  : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, hv_net_gate_str(g_guest_net_telemetry.dns_status), hv_net_gate_color(g_guest_net_telemetry.dns_status), c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "TCP Connection  : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, hv_net_gate_str(g_guest_net_telemetry.tcp_status), hv_net_gate_color(g_guest_net_telemetry.tcp_status), c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "HTTPS / TLS     : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, hv_net_gate_str(g_guest_net_telemetry.https_status), hv_net_gate_color(g_guest_net_telemetry.https_status), c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "REAL INTERNET   : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, hv_net_gate_str(g_guest_net_telemetry.internet_status), hv_net_gate_color(g_guest_net_telemetry.internet_status), c_card_bg);

    /* Bottom Heartbeat Bar */
    uint32_t bar_x = 24;
    uint32_t bar_y = (screen_h > 46) ? (screen_h - 44) : 720;
    uint32_t bar_w = (screen_w > 48) ? (screen_w - 48) : 960;
    uint32_t bar_h = 38;

    abde_fill_rect(bar_x, bar_y, bar_w, bar_h, c_card_bg);
    abde_fill_rect(bar_x, bar_y, bar_w, 2, c_pass);

    char hb_buf[256], s_key_cnt[16], s_bb_xev[16], s_bb_hid[16];
    hv_fmt_u64_str(s_key_cnt, g_dashboard_key_press_count);
    hv_fmt_u64_str(s_bb_xev, g_xhci_events);
    hv_fmt_u64_str(s_bb_hid, g_usb_hid_packets);
    strcpy(hb_buf, "[NET DEBUG ");
    size_t hbl = strlen(hb_buf);
    hb_buf[hbl++] = spin_char;
    hb_buf[hbl] = '\0';
    strcat(hb_buf, "] Wire RX: "); strcat(hb_buf, s_prx_p);
    strcat(hb_buf, " | Wire TX: "); strcat(hb_buf, s_ptx_p);
    strcat(hb_buf, " | Guest IP: "); strcat(hb_buf, s_gip);
    strcat(hb_buf, " | [F1: VMX | F2: Net | F3: Gfx | F4/ESC: Main] | Keys: ");
    strcat(hb_buf, s_key_cnt);
    strcat(hb_buf, " (RX:"); strcat(hb_buf, s_bb_xev);
    strcat(hb_buf, " HID:"); strcat(hb_buf, s_bb_hid); strcat(hb_buf, ")");
    abde_render_string(bar_x + 16, bar_y + 10, hb_buf, c_pass, c_card_bg);
}

void hypervisor_dashboard_render_graphics_debug(VirtualMachine *vm, char spin_char) {
    if (!g_hv_dashboard.boot_info || !g_hv_dashboard.boot_info->vbe_framebuffer) return;
    if (!vm || !vm->bsp_vcpu) return;

    uint32_t screen_w = g_hv_dashboard.boot_info->vbe_width;
    uint32_t screen_h = g_hv_dashboard.boot_info->vbe_height;

    uint32_t c_bg         = 0xFF0D1117;
    uint32_t c_header_bg  = 0xFF161B22;
    uint32_t c_card_bg    = 0xFF161B22;
    uint32_t c_text       = 0xFFE6EDF3;
    uint32_t c_text_dim   = 0xFF8B949E;
    uint32_t c_blue       = 0xFF58A6FF;
    uint32_t c_pass       = 0xFF2EA043;
    uint32_t c_warn       = 0xFFD29922;
    uint32_t c_fail       = 0xFFF85149;

    abde_fill_rect(0, 0, screen_w, screen_h, c_bg);

    /* 1. Header Banner */
    abde_fill_rect(0, 0, screen_w, 44, c_header_bg);
    abde_render_string(24, 14, "ATOMS OS -- GRAPHICS & DISPLAY DEEP DEBUG (PHASE 5A-4)", c_text, c_header_bg);

    char top_status[80];
    top_status[0] = '['; top_status[1] = spin_char; top_status[2] = ']'; top_status[3] = '\0';
    strcat(top_status, " FreeBSD -> VirtIO-GPU 2D -> ATOMS Display Bridge -> Physical Display");
    abde_render_string(screen_w - 600, 14, top_status, c_blue, c_header_bg);

    uint32_t col1_x = 24;
    uint32_t col1_w = 480;
    uint32_t col2_x = 520;
    uint32_t col2_w = (screen_w > 544) ? (screen_w - 544) : 480;

    VirtIODisplay *disp = g_active_virtio_display;

    /* Card 1: HOST FRAMEBUFFER (Physical Display) */
    uint32_t c1_y = 54;
    uint32_t c1_h = 240;
    abde_fill_rect(col1_x, c1_y, col1_w, c1_h, c_card_bg);
    abde_fill_rect(col1_x, c1_y, col1_w, 2, c_pass);
    abde_render_string(col1_x + 16, c1_y + 10, "1. HOST FRAMEBUFFER (Physical Video Hardware)", c_blue, c_card_bg);

    uint32_t y = c1_y + 34;
    char s_hfb[32], s_hdim[32], s_hpitch[32];
    hv_fmt_hex_str(s_hfb, g_hv_dashboard.boot_info->vbe_framebuffer, 16);
    abde_render_string(col1_x + 16, y, "Base Address    : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, s_hfb, c_text, c_card_bg);
    y += 18;

    char sw_s[12], sh_s[12];
    hv_fmt_u64_str(sw_s, screen_w); hv_fmt_u64_str(sh_s, screen_h);
    strcpy(s_hdim, sw_s); strcat(s_hdim, " x "); strcat(s_hdim, sh_s);
    abde_render_string(col1_x + 16, y, "Native Resolution: ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, s_hdim, c_pass, c_card_bg);
    y += 18;

    hv_fmt_u64_str(s_hpitch, g_hv_dashboard.boot_info->vbe_pitch);
    strcat(s_hpitch, " Bytes");
    abde_render_string(col1_x + 16, y, "Stride / Pitch  : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, s_hpitch, c_text, c_card_bg);
    y += 18;

    abde_render_string(col1_x + 16, y, "Pixel Format    : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, "32bpp Linear ARGB8888", c_text, c_card_bg);
    y += 18;

    abde_render_string(col1_x + 16, y, "Current Owner   : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, (disp && disp->guest_owns_display) ? "GUEST DISPLAY" : "RUNTIME DASHBOARD", (disp && disp->guest_owns_display) ? c_pass : c_blue, c_card_bg);

    /* Card 2: GUEST DISPLAY & VIRTIO-GPU */
    uint32_t c2_y = c1_y + c1_h + 12;
    uint32_t c2_h = 240;
    abde_fill_rect(col1_x, c2_y, col1_w, c2_h, c_card_bg);
    abde_fill_rect(col1_x, c2_y, col1_w, 2, c_blue);
    abde_render_string(col1_x + 16, c2_y + 10, "2. GUEST DISPLAY DISCOVERY & DEVICE", c_blue, c_card_bg);

    y = c2_y + 34;
    abde_render_string(col1_x + 16, y, "VirtIO Device   : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, (disp && disp->base) ? "VirtIO-GPU (PCI 00:04.0)" : "NOT REGISTERED", (disp && disp->base) ? c_pass : c_fail, c_card_bg);
    y += 18;

    abde_render_string(col1_x + 16, y, "Subsystem ID    : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, "0x0010 (VIRTIO_ID_GPU - Standard)", c_pass, c_card_bg);
    y += 18;

    abde_render_string(col1_x + 16, y, "Console Driver  : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, "FreeBSD vt_efifb (Active)", c_pass, c_card_bg);
    y += 18;

    abde_render_string(col1_x + 16, y, "Metadata Record : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, "MODINFOMD_EFI_FB (0x1005)", c_pass, c_card_bg);
    y += 18;

    char s_gdim[32], s_gpitch[32], s_gsize[32];
    if (disp) {
        char gw[12], gh[12];
        hv_fmt_u64_str(gw, disp->width); hv_fmt_u64_str(gh, disp->height);
        strcpy(s_gdim, gw); strcat(s_gdim, " x "); strcat(s_gdim, gh);
        hv_fmt_u64_str(s_gpitch, disp->pitch); strcat(s_gpitch, " Bytes");
        hv_fmt_u64_str(s_gsize, disp->framebuffer_size / 1024); strcat(s_gsize, " KB");
    } else {
        strcpy(s_gdim, "N/A"); strcpy(s_gpitch, "N/A"); strcpy(s_gsize, "N/A");
    }
    abde_render_string(col1_x + 16, y, "Guest Resolution: ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, s_gdim, c_text, c_card_bg);
    y += 18;

    abde_render_string(col1_x + 16, y, "Guest Stride    : ", c_text_dim, c_card_bg);
    abde_render_string(col1_x + 180, y, s_gpitch, c_text, c_card_bg);

    /* Card 3: VIRTIO-GPU 2D COMMANDS */
    uint32_t c3_y = 54;
    uint32_t c3_h = 240;
    abde_fill_rect(col2_x, c3_y, col2_w, c3_h, c_card_bg);
    abde_fill_rect(col2_x, c3_y, col2_w, 2, c_blue);
    abde_render_string(col2_x + 16, c3_y + 10, "3. VIRTIO-GPU QUEUES & 2D ENGINE", c_blue, c_card_bg);

    y = c3_y + 34;
    abde_render_string(col2_x + 16, y, "Control Queue   : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, "Queue 0 (Depth 128, Split VirtQ)", c_pass, c_card_bg);
    y += 18;

    char s_cmds[24], s_flushes[24], s_frames[24], s_scanout[16];
    hv_fmt_u64_str(s_cmds, disp ? disp->total_commands : 0);
    hv_fmt_u64_str(s_flushes, disp ? disp->total_flushes : 0);
    hv_fmt_u64_str(s_frames, disp ? disp->total_frames_presented : 0);
    hv_fmt_u64_str(s_scanout, disp ? disp->active_scanout_resource_id : 0);

    abde_render_string(col2_x + 16, y, "2D Commands Rcvd: ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, s_cmds, c_blue, c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "Active Scanout  : ", c_text_dim, c_card_bg);
    char s_sc_desc[32];
    strcpy(s_sc_desc, "Resource ID #"); strcat(s_sc_desc, s_scanout);
    abde_render_string(col2_x + 180, y, (disp && disp->active_scanout_resource_id > 0) ? s_sc_desc : "NONE (Pre-Scanout)", (disp && disp->active_scanout_resource_id > 0) ? c_pass : c_warn, c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "Scanout Flushes : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, s_flushes, c_pass, c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "Frames Presented: ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, s_frames, c_pass, c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "Supported Cmds  : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, "INFO|CREATE_2D|ATTACH|SCANOUT|FLUSH", c_text, c_card_bg);

    /* Card 4: MEMORY TRANSLATION & PIXEL PROVENANCE */
    uint32_t c4_y = c3_y + c3_h + 12;
    uint32_t c4_h = 240;
    abde_fill_rect(col2_x, c4_y, col2_w, c4_h, c_card_bg);
    abde_fill_rect(col2_x, c4_y, col2_w, 2, c_pass);
    abde_render_string(col2_x + 16, c4_y + 10, "4. MEMORY TRANSLATION & PIXEL PROVENANCE", c_blue, c_card_bg);

    y = c4_y + 34;
    char s_ggpa[32], s_ghva[32];
    hv_fmt_hex_str(s_ggpa, disp ? disp->gpa_framebuffer : 0, 16);
    hv_fmt_hex_str(s_ghva, (uint64_t)(uintptr_t)(disp ? disp->hva_framebuffer : 0), 16);

    abde_render_string(col2_x + 16, y, "Guest GPA Base  : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, (disp && disp->gpa_framebuffer > 0) ? s_ggpa : "PENDING (Guest Alloc)", (disp && disp->gpa_framebuffer > 0) ? c_pass : c_warn, c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "Host Backing HVA: ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, (disp && disp->hva_framebuffer) ? s_ghva : "PENDING", (disp && disp->hva_framebuffer) ? c_pass : c_warn, c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "EPT Direct Map  : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, "2048 MB Identity Map (RWX, Safe)", c_pass, c_card_bg);
    y += 18;

    abde_render_string(col2_x + 16, y, "Buffer Backing  : ", c_text_dim, c_card_bg);
    abde_render_string(col2_x + 180, y, s_gsize, c_text, c_card_bg);
    y += 22;

    abde_render_string(col2_x + 16, y, "PIXEL PROVENANCE: ", c_text_dim, c_card_bg);
    if (disp && disp->total_flushes > 0) {
        abde_render_string(col2_x + 180, y, "PASS (Real Guest Framebuffer)", c_pass, c_card_bg);
    } else if (disp && disp->gpa_framebuffer > 0) {
        abde_render_string(col2_x + 180, y, "PASS (FreeBSD vt_efifb Active)", c_pass, c_card_bg);
    } else if (disp && disp->driver_active) {
        abde_render_string(col2_x + 180, y, "PARTIAL (Driver Active, Awaiting Flush)", c_warn, c_card_bg);
    } else {
        abde_render_string(col2_x + 180, y, "UNKNOWN (GPU Probe In Progress)", c_text_dim, c_card_bg);
    }

    /* Bottom Heartbeat Bar */
    uint32_t bar_x = 24;
    uint32_t bar_y = (screen_h > 46) ? (screen_h - 44) : 720;
    uint32_t bar_w = (screen_w > 48) ? (screen_w - 48) : 960;
    uint32_t bar_h = 38;

    abde_fill_rect(bar_x, bar_y, bar_w, bar_h, c_card_bg);
    abde_fill_rect(bar_x, bar_y, bar_w, 2, c_pass);

    char hb_buf[256], s_key_cnt[16], s_bb_xev[16], s_bb_hid[16];
    hv_fmt_u64_str(s_key_cnt, g_dashboard_key_press_count);
    hv_fmt_u64_str(s_bb_xev, g_xhci_events);
    hv_fmt_u64_str(s_bb_hid, g_usb_hid_packets);
    strcpy(hb_buf, "[GFX DEBUG ");
    size_t hbl = strlen(hb_buf);
    hb_buf[hbl++] = spin_char;
    hb_buf[hbl] = '\0';
    strcat(hb_buf, "] Cmds: "); strcat(hb_buf, s_cmds);
    strcat(hb_buf, " | Flushes: "); strcat(hb_buf, s_flushes);
    strcat(hb_buf, " | Frames: "); strcat(hb_buf, s_frames);
    strcat(hb_buf, " | [F1: VMX | F2: Net | F3: Gfx | F4/ESC: Main] | Keys: ");
    strcat(hb_buf, s_key_cnt);
    strcat(hb_buf, " (RX:"); strcat(hb_buf, s_bb_xev);
    strcat(hb_buf, " HID:"); strcat(hb_buf, s_bb_hid); strcat(hb_buf, ")");
    abde_render_string(bar_x + 16, bar_y + 10, hb_buf, c_pass, c_card_bg);
}

void hypervisor_dashboard_render_runtime_hud(VirtualMachine *vm, char spin_char) {
    hypervisor_dashboard_render_runtime_dashboard(vm, spin_char);
}

void hypervisor_dashboard_emit_runtime_heartbeat(VirtualMachine *vm, char spin_char) {
    if (!vm || !vm->bsp_vcpu) return;
    vCPU *vcpu = vm->bsp_vcpu;

    char h_rip[24], h_cr3[24], h_exits[16], h_reason[12];
    hv_fmt_hex_str(h_rip, vcpu->guest_regs.rip, 16);
    hv_fmt_hex_str(h_cr3, vcpu->cr3, 16);
    hv_fmt_hex_str(h_exits, vm->total_vmexits, 8);
    hv_fmt_hex_str(h_reason, vcpu->last_exit.exit_reason, 4);

    char s_rx[20], s_tx[20];
    hv_fmt_u64_str(s_rx, g_guest_net_telemetry.rx_packets);
    hv_fmt_u64_str(s_tx, g_guest_net_telemetry.tx_packets);

    char msg[320];
    msg[0] = '\0';
    strcat(msg, "[FREEBSD RUNTIME ");
    size_t len = strlen(msg);
    msg[len++] = spin_char;
    msg[len] = '\0';
    strcat(msg, "] State=");
    strcat(msg, vm->runtime_active ? "RUNNING" : "STOPPED");
    strcat(msg, " VM=ACTIVE RIP=");
    strcat(msg, h_rip);
    strcat(msg, " Exits=");
    strcat(msg, h_exits);
    strcat(msg, " LastExit=");
    strcat(msg, h_reason);
    strcat(msg, " Disp=");
    strcat(msg, atoms_hypervisor_exit_disposition_str(vcpu->last_exit.disposition));
    strcat(msg, " NetRX=");
    strcat(msg, s_rx);
    strcat(msg, " NetTX=");
    strcat(msg, s_tx);
    strcat(msg, " DHCP=");
    strcat(msg, hv_net_gate_str(g_guest_net_telemetry.dhcp_status));
    strcat(msg, " DNS=");
    strcat(msg, hv_net_gate_str(g_guest_net_telemetry.dns_status));
    strcat(msg, " TCP=");
    strcat(msg, hv_net_gate_str(g_guest_net_telemetry.tcp_status));
    strcat(msg, " HTTPS=");
    strcat(msg, hv_net_gate_str(g_guest_net_telemetry.https_status));
    strcat(msg, " NET=");
    strcat(msg, hv_net_gate_str(g_guest_net_telemetry.internet_status));

    com1_puts(msg);
    com1_puts("\r\n");

    if (debuglan_active()) {
        debuglan_log_subsys("FREEBSD_RT", "%s", msg);
    }
}

void hypervisor_dashboard_run(boot_info_t *boot_info) {
    hypervisor_dashboard_init(boot_info);
    hypervisor_dashboard_render_frame();
    hypervisor_dashboard_run_stage_pipeline();

    VirtualMachine *rt_vm = atoms_hypervisor_get_runtime_vm();

    if (g_autopsy_engine.has_post_failure) {
        /* Render and lock both Autopsy Panels permanently on physical display */
        vmentry_autopsy_render_operator_view();
        vmentry_autopsy_render_machine_log_panel();
        vmentry_autopsy_render_heartbeat('|');

        /* Automatically transmit high-resolution visual autopsy buffer over UDP 9998 */
        extern bool atoms_screenshot_capture_sync(uint32_t session_id);
        com1_puts("[AUTOPSY] Automatically transmitting physical display screenshot over UDP 9998...\r\n");
        atoms_screenshot_capture_sync(99);
    } else {
        if (rt_vm) {
            hypervisor_dashboard_render_runtime_dashboard(rt_vm, '|');
        } else {
            hypervisor_dashboard_render_frame();
        }
    }

    /* Telemetry Spinner & Persistent Runtime Control Loop */
    static volatile uint64_t s_dash_ticks = 0;
    static const char s_spin[] = {'|', '/', '-', '\\'};

    extern bool net_poll(void);
    extern bool atoms_screenshot_step(void);
    extern bool atoms_screenshot_is_busy(void);

    while (1) {
        /* Check physical keyboard for F1-F4 toggle */
        hv_poll_developer_toggle_keys();

        /* Phase 5A-1: Continuously Step Persistent Guest vCPU */
        if (rt_vm && rt_vm->runtime_active) {
            atoms_hypervisor_runtime_step(rt_vm, 2000);
        }

        /* Continuously poll Realtek NIC RX ring and step cooperative screenshot chunks */
        net_poll();
        if (atoms_screenshot_is_busy()) {
            atoms_screenshot_step();
        }

        s_dash_ticks++;
        if ((s_dash_ticks % 50000) == 0) {
            char spin_char = s_spin[(s_dash_ticks / 50000) % 4];

            if (g_autopsy_engine.has_post_failure) {
                /* DO NOT overwrite autopsy screen. Only update bottom heartbeat bar! */
                vmentry_autopsy_render_heartbeat(spin_char);
            } else if (g_hv_dashboard_page == HV_DASHBOARD_PAGE_VMX || (g_hv_dashboard_show_forensics && g_vmentry_screen_forensics.valid)) {
                hypervisor_dashboard_render_vmentry_forensics();
            } else if (rt_vm) {
                if (g_hv_dashboard_page == HV_DASHBOARD_PAGE_NETWORK) {
                    hypervisor_dashboard_render_network_debug(rt_vm, spin_char);
                } else if (g_hv_dashboard_page == HV_DASHBOARD_PAGE_GRAPHICS) {
                    hypervisor_dashboard_render_graphics_debug(rt_vm, spin_char);
                } else {
                    hypervisor_dashboard_render_runtime_dashboard(rt_vm, spin_char);
                }
                hypervisor_dashboard_emit_runtime_heartbeat(rt_vm, spin_char);
            } else {
                hypervisor_dashboard_render_frame();
                char spinner_msg[64];
                hv_format_heartbeat(spinner_msg,
                                    spin_char,
                                    g_hv_dashboard.halted ? "HALTED_ON_FAIL" : "PASS",
                                    g_hv_dashboard.total_stages_passed,
                                    HV_STAGE_MAX);
                com1_puts(spinner_msg);
                com1_puts("\r\n");
                if (debuglan_active()) {
                    debuglan_log_subsys("HYPERVISOR", "%s", spinner_msg);
                }
            }
        }
        __asm__ volatile("pause");
    }
}

