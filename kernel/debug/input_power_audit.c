#include "input_power_audit.h"
#include "kernel/debug/aipdebug/aipdebug.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/display/dgl/include/dgl.h"
#include "kernel/core/bram/include/bram.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/core/pci/pci.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"

InputPowerAudit g_input_power_audit;

static inline uint64_t audit_rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static inline void com1_putc(char c) {
    while ((io_in8(0x3F8 + 5) & 0x20) == 0);
    io_out8(0x3F8, c);
}

static void com1_puts(const char *s) {
    while (*s) {
        if (*s == '\n') com1_putc('\r');
        com1_putc(*s++);
    }
}

static void com1_put_hex_byte(uint8_t val) {
    const char hex[] = "0123456789ABCDEF";
    com1_putc(hex[(val >> 4) & 0xF]);
    com1_putc(hex[val & 0xF]);
}

static void com1_put_hex_dword(uint32_t val) {
    for (int i = 28; i >= 0; i -= 4) {
        com1_putc("0123456789ABCDEF"[(val >> i) & 0xF]);
    }
}

static void com1_put_dec(uint32_t val) {
    if (val == 0) { com1_putc('0'); return; }
    char buf[12]; int pos = 10; buf[11] = '\0';
    while (val > 0) { buf[pos--] = '0' + (val % 10); val /= 10; }
    com1_puts(&buf[pos + 1]);
}

// =====================================================================
// PART 1: PASSIVE 8042 STATUS CAPTURE (100 SAMPLES)
// =====================================================================
static void audit_capture_8042_passive(void) {
    com1_puts("[INPUT-AUDIT] Starting 100-sample passive sampling of Port 0x64...\r\n");

    for (int i = 0; i < AUDIT_STATUS_SAMPLES; i++) {
        uint64_t t = audit_rdtsc();
        uint8_t s = io_in8(0x64);

        g_input_power_audit.status_samples[i].tsc = t;
        g_input_power_audit.status_samples[i].status = s;
        g_input_power_audit.status_samples[i].obf      = (s & (1 << 0)) != 0;
        g_input_power_audit.status_samples[i].ibf      = (s & (1 << 1)) != 0;
        g_input_power_audit.status_samples[i].sys      = (s & (1 << 2)) != 0;
        g_input_power_audit.status_samples[i].cmd_data = (s & (1 << 3)) != 0;
        g_input_power_audit.status_samples[i].keylock  = (s & (1 << 4)) != 0;
        g_input_power_audit.status_samples[i].aux      = (s & (1 << 5)) != 0;
        g_input_power_audit.status_samples[i].timeout  = (s & (1 << 6)) != 0;
        g_input_power_audit.status_samples[i].parity   = (s & (1 << 7)) != 0;

        // ONLY read Port 0x60 if OBF is actually asserted
        if (s & 1) {
            uint8_t d = io_in8(0x60);
            uint8_t s_after = io_in8(0x64);
            g_input_power_audit.status_samples[i].has_data = true;
            g_input_power_audit.status_samples[i].data_byte = d;
            g_input_power_audit.status_samples[i].status_after_read = s_after;

            com1_puts("[INPUT-AUDIT] 8042 OBF SET! Sample=");
            com1_put_dec(i);
            com1_puts(" Byte=0x");
            com1_put_hex_byte(d);
            com1_puts(" AUX=");
            com1_puts(g_input_power_audit.status_samples[i].aux ? "1 (MOUSE)" : "0 (KBD)");
            com1_puts(" StatusAfter=0x");
            com1_put_hex_byte(s_after);
            com1_puts("\r\n");
        } else {
            g_input_power_audit.status_samples[i].has_data = false;
        }

        // Small microsecond delay between samples
        for (volatile int d = 0; d < 2000; d++) { __asm__ volatile("pause"); }
    }

    g_input_power_audit.initial_8042_timeout = g_input_power_audit.status_samples[0].timeout;
    g_input_power_audit.initial_8042_obf     = g_input_power_audit.status_samples[0].obf;
    g_input_power_audit.initial_8042_aux     = g_input_power_audit.status_samples[0].aux;

    com1_puts("[INPUT-AUDIT] 8042 Passive Capture Complete. Initial Status=0x");
    com1_put_hex_byte(g_input_power_audit.status_samples[0].status);
    com1_puts(" [TIMEOUT=");
    com1_puts(g_input_power_audit.initial_8042_timeout ? "SET" : "CLEARED");
    com1_puts(" OBF=");
    com1_puts(g_input_power_audit.initial_8042_obf ? "1" : "0");
    com1_puts(" AUX=");
    com1_puts(g_input_power_audit.initial_8042_aux ? "1" : "0");
    com1_puts("]\r\n");
}

// =====================================================================
// PART 2: PASSIVE PCI & xHCI REGISTER / PORT POWER SAMPLING
// =====================================================================
static void audit_sample_xhci_passive(void) {
    com1_puts("[INPUT-AUDIT] Scanning PCI bus for USB Host Controller (Class 0x0C, Subclass 0x03)...\r\n");

    uint32_t dev_count = pci_get_device_count();
    PCIDevice *xhci_dev = 0;

    for (uint32_t i = 0; i < dev_count; i++) {
        PCIDevice *dev = pci_get_device(i);
        if (dev->base_class == 0x0C && dev->sub_class == 0x03) {
            uint32_t class_info = pci_read_config(dev->bus, dev->slot, dev->func, 0x08);
            uint8_t prog_if = (class_info >> 8) & 0xFF;
            if (prog_if == 0x30) { // xHCI
                xhci_dev = dev;
                break;
            }
        }
    }

    if (!xhci_dev) {
        com1_puts("[INPUT-AUDIT] [FAIL] No xHCI controller detected on PCI bus!\r\n");
        g_input_power_audit.pci_controller_found = false;
        return;
    }

    g_input_power_audit.pci_controller_found = true;
    g_input_power_audit.pci_bus = xhci_dev->bus;
    g_input_power_audit.pci_slot = xhci_dev->slot;
    g_input_power_audit.pci_func = xhci_dev->func;
    g_input_power_audit.vendor_id = xhci_dev->vendor_id;
    g_input_power_audit.device_id = xhci_dev->device_id;

    // Read PCI Command & Status registers
    g_input_power_audit.pci_cmd = pci_read_config_16(xhci_dev->bus, xhci_dev->slot, xhci_dev->func, 0x04);
    g_input_power_audit.pci_status = pci_read_config_16(xhci_dev->bus, xhci_dev->slot, xhci_dev->func, 0x06);
    g_input_power_audit.irq_line = pci_read_config_8(xhci_dev->bus, xhci_dev->slot, xhci_dev->func, 0x3C);
    g_input_power_audit.irq_pin = pci_read_config_8(xhci_dev->bus, xhci_dev->slot, xhci_dev->func, 0x3D);

    // Read BAR0 (offset 0x10) and BAR1 (offset 0x14)
    uint32_t bar0 = pci_read_config(xhci_dev->bus, xhci_dev->slot, xhci_dev->func, 0x10);
    uint64_t mmio = bar0 & 0xFFFFFFF0;
    if ((bar0 & 0x06) == 0x04) { // 64-bit BAR
        uint32_t bar1 = pci_read_config(xhci_dev->bus, xhci_dev->slot, xhci_dev->func, 0x14);
        mmio |= ((uint64_t)bar1 << 32);
    }
    g_input_power_audit.mmio_base = mmio;

    com1_puts("[INPUT-AUDIT] xHCI Controller: B:S:F=");
    com1_put_dec(xhci_dev->bus); com1_putc(':'); com1_put_dec(xhci_dev->slot); com1_putc('.'); com1_put_dec(xhci_dev->func);
    com1_puts(" VID:DID=0x"); com1_put_hex_dword(((uint32_t)xhci_dev->vendor_id << 16) | xhci_dev->device_id);
    com1_puts(" PCI_CMD=0x"); com1_put_hex_byte(g_input_power_audit.pci_cmd >> 8); com1_put_hex_byte(g_input_power_audit.pci_cmd & 0xFF);
    com1_puts(" MMIO=0x"); com1_put_hex_dword((uint32_t)(mmio >> 32)); com1_put_hex_dword((uint32_t)mmio);
    com1_puts("\r\n");

    // Map MMIO pages (2MB region) to allow safe reading
    extern void* vmm_get_kernel_pml4(void);
    void* pml4 = vmm_get_active_pml4();
    void* k_pml4 = vmm_get_kernel_pml4();
    for (uint64_t p = 0; p < 512; p++) {
        uint64_t phys = (mmio + p * 4096) & PAGE_PHYS_ADDRESS_MASK;
        vmm_map_page(pml4, phys, phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        if (k_pml4 && k_pml4 != pml4) {
            vmm_map_page(k_pml4, phys, phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        }
    }

    // Passive Read: Capability Registers
    volatile uint32_t* cap_regs32 = (volatile uint32_t*)mmio;
    uint32_t cap_dw0 = cap_regs32[0];
    g_input_power_audit.caplength = cap_dw0 & 0xFF;
    g_input_power_audit.hciversion = (cap_dw0 >> 16) & 0xFFFF;

    uint32_t hcsparams1 = cap_regs32[1];
    g_input_power_audit.max_slots = hcsparams1 & 0xFF;
    g_input_power_audit.max_ports = (hcsparams1 >> 24) & 0xFF;

    uint32_t hccparams1 = cap_regs32[4];
    uint32_t eecp = (hccparams1 >> 16) & 0xFFFF;
    g_input_power_audit.eecp_offset = eecp << 2;

    com1_puts("[INPUT-AUDIT] xHCI CAP: Version=0x"); com1_put_hex_byte(g_input_power_audit.hciversion >> 8); com1_put_hex_byte(g_input_power_audit.hciversion & 0xFF);
    com1_puts(" CapLength="); com1_put_dec(g_input_power_audit.caplength);
    com1_puts(" MaxSlots="); com1_put_dec(g_input_power_audit.max_slots);
    com1_puts(" MaxPorts="); com1_put_dec(g_input_power_audit.max_ports);
    com1_puts(" EECP=0x"); com1_put_hex_dword(g_input_power_audit.eecp_offset);
    com1_puts("\r\n");

    // Passive Read: USBLEGSUP (Ownership & SMM state)
    if (eecp) {
        uint32_t cur_offset = eecp << 2;
        int guard = 0;
        while (cur_offset && guard++ < 16) {
            volatile uint32_t* ext_cap = (volatile uint32_t*)(mmio + cur_offset);
            uint32_t val = ext_cap[0];
            uint8_t cap_id = val & 0xFF;
            uint8_t next_eecp = (val >> 8) & 0xFF;

            if (cap_id == 1) { // USBLEGSUP
                g_input_power_audit.has_legacy_ext = true;
                g_input_power_audit.raw_usblegsup = val;
                g_input_power_audit.bios_owned = (val & (1 << 16)) != 0;
                g_input_power_audit.os_owned = (val & (1 << 24)) != 0;
                g_input_power_audit.raw_usblegctlsts = ext_cap[1];

                com1_puts("[INPUT-AUDIT] USBLEGSUP: Raw=0x"); com1_put_hex_dword(val);
                com1_puts(" [BIOS_OWNED="); com1_puts(g_input_power_audit.bios_owned ? "YES" : "NO");
                com1_puts(" OS_OWNED="); com1_puts(g_input_power_audit.os_owned ? "YES" : "NO");
                com1_puts("] USBLEGCTLSTS=0x"); com1_put_hex_dword(ext_cap[1]);
                com1_puts("\r\n");
                break;
            }
            if (!next_eecp) break;
            cur_offset += (next_eecp << 2);
        }
    }

    // Passive Read: Operational Registers
    volatile uint8_t* cap_regs8 = (volatile uint8_t*)mmio;
    volatile uint32_t* op_regs = (volatile uint32_t*)(cap_regs8 + g_input_power_audit.caplength);
    g_input_power_audit.usbcmd = op_regs[0];
    g_input_power_audit.usbsts = op_regs[1];
    g_input_power_audit.config_reg = op_regs[14];

    g_input_power_audit.is_running   = (g_input_power_audit.usbcmd & (1 << 0)) != 0;
    g_input_power_audit.is_halted    = (g_input_power_audit.usbsts & (1 << 0)) != 0;
    g_input_power_audit.host_sys_err = (g_input_power_audit.usbsts & (1 << 2)) != 0;
    g_input_power_audit.not_ready    = (g_input_power_audit.usbsts & (1 << 11)) != 0;

    com1_puts("[INPUT-AUDIT] xHCI OPREGS: USBCMD=0x"); com1_put_hex_dword(g_input_power_audit.usbcmd);
    com1_puts(" [RUN/STOP="); com1_puts(g_input_power_audit.is_running ? "RUN" : "STOP");
    com1_puts("] USBSTS=0x"); com1_put_hex_dword(g_input_power_audit.usbsts);
    com1_puts(" [HALTED="); com1_puts(g_input_power_audit.is_halted ? "YES" : "NO");
    com1_puts(" CNR="); com1_puts(g_input_power_audit.not_ready ? "NOT_READY" : "READY");
    com1_puts("]\r\n");

    // Passive Read: Per-Port PORTSC Registers (Port Power & Device Connection)
    uint32_t num_ports = g_input_power_audit.max_ports;
    if (num_ports > AUDIT_MAX_PORTS) num_ports = AUDIT_MAX_PORTS;
    g_input_power_audit.port_count = num_ports;
    g_input_power_audit.total_connected_usb_devices = 0;
    g_input_power_audit.total_powered_usb_ports = 0;

    volatile uint32_t* port_regs = (volatile uint32_t*)(cap_regs8 + g_input_power_audit.caplength + 0x400);

    for (uint32_t p = 0; p < num_ports; p++) {
        uint32_t portsc = port_regs[p * 4];
        AuditPortInfo *pi = &g_input_power_audit.ports[p];
        pi->port_num = p + 1;
        pi->raw_portsc = portsc;
        pi->ccs   = (portsc & (1 << 0)) != 0;
        pi->ped   = (portsc & (1 << 1)) != 0;
        pi->oca   = (portsc & (1 << 3)) != 0;
        pi->pr    = (portsc & (1 << 4)) != 0;
        pi->pls   = (portsc >> 5) & 0xF;
        pi->pp    = (portsc & (1 << 9)) != 0; // BIT 9: PORT POWER!
        pi->speed = (portsc >> 10) & 0xF;

        if (pi->ccs) g_input_power_audit.total_connected_usb_devices++;
        if (pi->pp)  g_input_power_audit.total_powered_usb_ports++;

        com1_puts("[INPUT-AUDIT] Port ");
        if (p + 1 < 10) com1_putc(' ');
        com1_put_dec(p + 1);
        com1_puts(": PORTSC=0x"); com1_put_hex_dword(portsc);
        com1_puts(" [PP="); com1_puts(pi->pp ? "POWER_ON" : "POWER_OFF");
        com1_puts(" CCS="); com1_puts(pi->ccs ? "CONNECTED" : "DISCONNECTED");
        com1_puts(" SPEED="); com1_put_dec(pi->speed);
        com1_puts(" PLS="); com1_put_dec(pi->pls);
        com1_puts("]\r\n");
    }

    com1_puts("[INPUT-AUDIT] USB Root Hub Summary: Total Ports=");
    com1_put_dec(num_ports);
    com1_puts(" Powered=");
    com1_put_dec(g_input_power_audit.total_powered_usb_ports);
    com1_puts(" ConnectedDevices=");
    com1_put_dec(g_input_power_audit.total_connected_usb_devices);
    com1_puts("\r\n");
}

// =====================================================================
// PART 3: ABDE DASHBOARD RENDERING
// =====================================================================
static void audit_render_dashboard(void) {
    uint32_t panel_bg    = 0x00080E1A; // Dark Navy Slate
    uint32_t header_bg   = 0x00101C30;
    uint32_t text_color  = 0x00E2E8F0;
    uint32_t label_color = 0x0094A3B8;
    uint32_t pass_color  = 0x0022C55E; // Emerald
    uint32_t fail_color  = 0x00EF4444; // Ruby
    uint32_t warn_color  = 0x00F59E0B; // Amber
    uint32_t cyan_color  = 0x0006B6D4;

    abde_fill_rect(0, 0, 1920, 1080, 0x00020617);
    abde_fill_rect(20, 20, 1880, 1040, panel_bg);

    // Title Bar
    abde_fill_rect(20, 20, 1880, 48, header_bg);
    abde_render_string(36, 36, "ATOMS OS — EMERGENCY INPUT POWER & CONTROLLER FORENSIC AUDIT", cyan_color, header_bg);
    abde_render_string(1400, 36, "TARGET: ASUS B750M-K (i3-14100F)", text_color, header_bg);

    // Left Column: USB & xHCI Controller Power / Port Status
    int lx = 40;
    int ly = 80;

    abde_render_string(lx, ly, "1. USB HOST CONTROLLER & VBUS POWER FORENSICS", cyan_color, panel_bg);
    ly += 22;

    abde_render_string(lx + 10, ly, "PCI Controller Location :", label_color, panel_bg);
    if (g_input_power_audit.pci_controller_found) {
        char pci_buf[32];
        pci_buf[0] = 'B'; pci_buf[1] = ':';
        pci_buf[2] = '0' + g_input_power_audit.pci_bus; pci_buf[3] = ' ';
        pci_buf[4] = 'D'; pci_buf[5] = ':';
        pci_buf[6] = '0' + (g_input_power_audit.pci_slot / 10);
        pci_buf[7] = '0' + (g_input_power_audit.pci_slot % 10);
        pci_buf[8] = ' '; pci_buf[9] = 'F'; pci_buf[10] = ':';
        pci_buf[11] = '0' + g_input_power_audit.pci_func; pci_buf[12] = '\0';
        abde_render_string(lx + 230, ly, pci_buf, pass_color, panel_bg);
    } else {
        abde_render_string(lx + 230, ly, "NOT FOUND", fail_color, panel_bg);
    }
    ly += 18;

    abde_render_string(lx + 10, ly, "PCI Command / Status    :", label_color, panel_bg);
    abde_render_string(lx + 230, ly, (g_input_power_audit.pci_cmd & 2) ? "MMIO ENABLED" : "MMIO DISABLED", (g_input_power_audit.pci_cmd & 2) ? pass_color : fail_color, panel_bg);
    ly += 18;

    abde_render_string(lx + 10, ly, "xHCI Controller State   :", label_color, panel_bg);
    abde_render_string(lx + 230, ly, g_input_power_audit.is_running ? "RUNNING (ACTIVE)" : "STOPPED / HALTED", g_input_power_audit.is_running ? pass_color : fail_color, panel_bg);
    ly += 18;

    abde_render_string(lx + 10, ly, "BIOS / OS Ownership     :", label_color, panel_bg);
    if (g_input_power_audit.bios_owned) {
        abde_render_string(lx + 230, ly, "BIOS SMM OWNED (LEGSUP=1)", warn_color, panel_bg);
    } else if (g_input_power_audit.os_owned) {
        abde_render_string(lx + 230, ly, "OS OWNED", pass_color, panel_bg);
    } else {
        abde_render_string(lx + 230, ly, "UNCLAIMED (DEFAULT)", text_color, panel_bg);
    }
    ly += 25;

    // Root Hub Port Power Table
    abde_render_string(lx, ly, "2. ROOT HUB PORT VBUS POWER & CONNECT STATUS", cyan_color, panel_bg);
    ly += 22;

    abde_render_string(lx + 10, ly, "Port", label_color, panel_bg);
    abde_render_string(lx + 70, ly, "VBUS Power (PP)", label_color, panel_bg);
    abde_render_string(lx + 220, ly, "Device (CCS)", label_color, panel_bg);
    abde_render_string(lx + 340, ly, "Speed", label_color, panel_bg);
    abde_render_string(lx + 430, ly, "Raw PORTSC", label_color, panel_bg);
    ly += 16;

    for (uint32_t p = 0; p < g_input_power_audit.port_count && p < 14; p++) {
        AuditPortInfo *pi = &g_input_power_audit.ports[p];
        char pnum[8]; pnum[0] = 'P'; pnum[1] = '0' + ((p+1)/10); pnum[2] = '0' + ((p+1)%10); pnum[3] = '\0';
        abde_render_string(lx + 10, ly, pnum, text_color, panel_bg);

        // Power Status
        abde_render_string(lx + 70, ly, pi->pp ? "POWER ON [OK]" : "POWER OFF [DEAD]", pi->pp ? pass_color : fail_color, panel_bg);

        // Connect Status
        abde_render_string(lx + 220, ly, pi->ccs ? "CONNECTED" : "NO DEVICE", pi->ccs ? pass_color : label_color, panel_bg);

        // Speed
        const char *spd = "NONE";
        if (pi->speed == 1) spd = "FULL (12M)";
        else if (pi->speed == 2) spd = "LOW (1.5M)";
        else if (pi->speed == 3) spd = "HIGH (480M)";
        else if (pi->speed == 4) spd = "SUPER (5G)";
        abde_render_string(lx + 340, ly, spd, text_color, panel_bg);

        // Raw hex
        char rhex[16];
        rhex[0] = '0'; rhex[1] = 'x';
        for (int b = 0; b < 8; b++) {
            rhex[2 + b] = "0123456789ABCDEF"[(pi->raw_portsc >> (28 - b * 4)) & 0xF];
        }
        rhex[10] = '\0';
        abde_render_string(lx + 430, ly, rhex, label_color, panel_bg);

        ly += 15;
    }

    // Right Column: 8042 Passive Status Log & Interpretation
    int rx = 980;
    int ry = 80;

    abde_render_string(rx, ry, "3. 8042 CONTROLLER PASSIVE STATUS (PRE-COMMAND BOOT)", cyan_color, panel_bg);
    ry += 22;

    abde_render_string(rx + 10, ry, "Boot Status Byte (0x64) :", label_color, panel_bg);
    char s_hex[8]; s_hex[0] = '0'; s_hex[1] = 'x';
    s_hex[2] = "0123456789ABCDEF"[(g_input_power_audit.status_samples[0].status >> 4) & 0xF];
    s_hex[3] = "0123456789ABCDEF"[g_input_power_audit.status_samples[0].status & 0xF];
    s_hex[4] = '\0';
    abde_render_string(rx + 230, ry, s_hex, text_color, panel_bg);
    ry += 18;

    abde_render_string(rx + 10, ry, "Bit 6 TIMEOUT Origin    :", label_color, panel_bg);
    abde_render_string(rx + 230, ry, g_input_power_audit.initial_8042_timeout ? "PRE-EXISTING AT BOOT (UEFI SMM)" : "CLEARED AT BOOT", g_input_power_audit.initial_8042_timeout ? warn_color : pass_color, panel_bg);
    ry += 18;

    abde_render_string(rx + 10, ry, "Bit 0 OBF (Output Data) :", label_color, panel_bg);
    abde_render_string(rx + 230, ry, g_input_power_audit.initial_8042_obf ? "PENDING BYTE PRESENT" : "BUFFER EMPTY (0 BYTES)", g_input_power_audit.initial_8042_obf ? warn_color : pass_color, panel_bg);
    ry += 18;

    abde_render_string(rx + 10, ry, "Bit 5 AUX (Mouse Data)  :", label_color, panel_bg);
    abde_render_string(rx + 230, ry, g_input_power_audit.initial_8042_aux ? "AUX ACTIVE" : "KEYBOARD ORIGIN", text_color, panel_bg);
    ry += 25;

    // Diagnosis & Forensic Determination Box
    abde_render_string(rx, ry, "4. EMPIRICAL FORENSIC VERDICT", cyan_color, panel_bg);
    ry += 22;

    bool usb_ports_powered = (g_input_power_audit.total_powered_usb_ports > 0);
    bool usb_devs_present  = (g_input_power_audit.total_connected_usb_devices > 0);

    abde_render_string(rx + 10, ry, "USB VBUS Rail Status    :", label_color, panel_bg);
    abde_render_string(rx + 230, ry, usb_ports_powered ? "POWERED (VBUS ENERGIZED)" : "UNPOWERED (VBUS OFF -> DEAD PERIPHERALS)", usb_ports_powered ? pass_color : fail_color, panel_bg);
    ry += 18;

    abde_render_string(rx + 10, ry, "USB Physical Devices    :", label_color, panel_bg);
    if (usb_devs_present) {
        abde_render_string(rx + 230, ry, "DEVICES DETECTED ON USB BUS", pass_color, panel_bg);
    } else {
        abde_render_string(rx + 230, ry, "NO DEVICES ON ROOT HUB", warn_color, panel_bg);
    }
    ry += 18;

    abde_render_string(rx + 10, ry, "Primary Cause of Dead Periph:", label_color, panel_bg);
    if (!usb_ports_powered) {
        abde_render_string(rx + 230, ry, "ROOT HUB PORT POWER (PP) IS OFF!", fail_color, panel_bg);
    } else if (!g_input_power_audit.is_running) {
        abde_render_string(rx + 230, ry, "xHCI CONTROLLER IS HALTED/STOPPED", fail_color, panel_bg);
    } else if (g_input_power_audit.bios_owned) {
        abde_render_string(rx + 230, ry, "UEFI BIOS SMM OWNS USB CONTROLLER", warn_color, panel_bg);
    } else {
        abde_render_string(rx + 230, ry, "AWAITING USB HID DRIVER POLLING", text_color, panel_bg);
    }
    ry += 25;
}

// =====================================================================
// MAIN AUDIT RUNNER
// =====================================================================
void input_power_audit_run(boot_info_t *boot_info) {
    (void)boot_info;
    bram_release_ownership(BRAM_RESOURCE_DISPLAY, BRAM_MODULE_ROOK_ENGINE);
    dgl_set_quiet_boot(false);
    dgl_set_state(DGL_STATE_RECOVERY);

    com1_puts("\r\n========================================================\r\n");
    com1_puts("[INPUT-AUDIT] EMERGENCY INPUT POWER & CONTROLLER FORENSIC PROBE\r\n");
    com1_puts("========================================================\r\n\r\n");

    // Initialize AI-(P)DEBUG telemetry
    aipd_init(AIPD_PROF_USB_XHCI | AIPD_PROF_PS2_KEYBOARD);

    // 1. Passive 8042 capture BEFORE touching any register
    audit_capture_8042_passive();

    // 2. Passive xHCI sampling
    audit_sample_xhci_passive();

    // 3. Render forensic dashboard
    audit_render_dashboard();

    // 4. Flush binary AI-(P)DEBUG packets over UDP 9997
    aipd_flush();

    // 5. Automated Forensic Screenshot: Stream 32-bit BMP over LAN to Python Receiver
    extern bool atoms_screenshot_capture_and_send(uint32_t session_id);
    atoms_screenshot_capture_and_send(1);
    aipd_flush();

    com1_puts("[INPUT-AUDIT] PROBE COMPLETE. ENTERING TELEMETRY LOOP...\r\n");

    uint64_t loop_counter = 0;
    for (;;) {
        loop_counter++;

        // Service Realtek PCIe NIC incoming frames (for remote SHUTDOWN / REBOOT packets on UDP 9999)
        if ((loop_counter % 1000) == 0) {
            extern void r8168_poll_receive(void);
            r8168_poll_receive();
        }

        __asm__ volatile("pause");
    }
}
