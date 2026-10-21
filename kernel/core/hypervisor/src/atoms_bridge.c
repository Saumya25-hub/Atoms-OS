/*
 * ATOMS OS — ATOMS Bridge: Guest Diagnostic Command Channel
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * UDP port 9998 command/response channel.  Commands are serviced
 * inside net_poll() which runs in VM-exit host context — VMCS is
 * never modified from outside the exit handler.
 *
 * Supported commands:
 *   state                – Full guest register & device state dump
 *   mem gpa <addr> <len> – Read guest physical memory (hex dump)
 *   mem gva <addr> <len> – Read guest virtual memory via CR3 walk
 *   idt <vec>            – Guest IDT handler address for vector
 *   rips                 – Last 256 exit RIPs (ring buffer dump)
 *   reboot               – Request guest reset
 */

#include "kernel/core/hypervisor/include/atoms_bridge.h"
#include "kernel/core/hypervisor/include/hypervisor.h"
#include "kernel/core/hypervisor/include/virtual_platform.h"
#include "kernel/core/hypervisor/include/guest_memory.h"
#include "kernel/net/udp/udp.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/debug/lan_debug/lan_debug.h"
#include "kernel/net/netif.h"

extern void com1_puts(const char *s);
extern VirtualMachine *atoms_hypervisor_get_runtime_vm(void);
extern bool atoms_hypervisor_translate_gva(vCPU *vcpu, uint64_t gva, uint8_t **out_hva);

/* --------------------------------------------------------------------------
 * Bridge State (file-scope)
 * -------------------------------------------------------------------------- */
static AtomsBridgeRipRing     s_rip_ring   = {0};
static AtomsBridgeExitHistory s_exit_hist  = {0};
static bool                   s_bridge_active = false;
static uint64_t               s_idtr_base  = 0;
static uint32_t               s_idtr_limit = 0;

/* --------------------------------------------------------------------------
 * Tiny hex/dec formatting helpers  (no libc dependency)
 * -------------------------------------------------------------------------- */
static const char s_hex[] = "0123456789ABCDEF";

static int bridge_hex64(char *dst, uint64_t v) {
    /* 16-char zero-padded hex */
    for (int i = 15; i >= 0; i--) {
        dst[i] = s_hex[v & 0xF];
        v >>= 4;
    }
    return 16;
}

static int bridge_hex32(char *dst, uint32_t v) {
    for (int i = 7; i >= 0; i--) {
        dst[i] = s_hex[v & 0xF];
        v >>= 4;
    }
    return 8;
}

static int bridge_hex8(char *dst, uint8_t v) {
    dst[0] = s_hex[(v >> 4) & 0xF];
    dst[1] = s_hex[v & 0xF];
    return 2;
}

static int bridge_dec(char *dst, uint64_t v) {
    char tmp[24];
    int n = 0;
    if (v == 0) { dst[0] = '0'; return 1; }
    while (v > 0) { tmp[n++] = '0' + (v % 10); v /= 10; }
    for (int i = 0; i < n; i++) dst[i] = tmp[n - 1 - i];
    return n;
}

/* Append literal string, return new position */
static int bcat(char *buf, int pos, const char *s) {
    while (*s) buf[pos++] = *s++;
    return pos;
}

/* --------------------------------------------------------------------------
 * Parse helpers
 * -------------------------------------------------------------------------- */
static bool str_startswith(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return false;
        s++; prefix++;
    }
    return true;
}

static uint64_t parse_hex_arg(const char *s) {
    uint64_t v = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    while (*s) {
        char c = *s++;
        uint8_t d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') d = 10 + (c - 'A');
        else break;
        v = (v << 4) | d;
    }
    return v;
}

static uint64_t parse_dec_arg(const char *s) {
    uint64_t v = 0;
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        s++;
    }
    return v;
}

/* Skip whitespace, return pointer to next token */
static const char *skip_ws(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

/* Find next whitespace-delimited token.  Returns pointer to first char
 * after the token. Sets *start and *len of the token. */
static const char *next_token(const char *s, const char **start, int *len) {
    s = skip_ws(s);
    *start = s;
    *len = 0;
    while (*s && *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') {
        s++; (*len)++;
    }
    return s;
}

/* --------------------------------------------------------------------------
 * Send UDP response back to the command sender
 * -------------------------------------------------------------------------- */
static void bridge_reply(uint32_t dest_ip, uint16_t dest_port,
                         const char *data, uint16_t len) {
    uint32_t src_ip = netif_get_ip();
    if (src_ip == 0) src_ip = LAN_DEBUG_HOST_IP;
    udp_send(src_ip, dest_ip,
             ATOMS_BRIDGE_PORT, dest_port,
             data, len);
}

/* --------------------------------------------------------------------------
 * Command: "state"
 * -------------------------------------------------------------------------- */
static void bridge_cmd_state(uint32_t src_ip, uint16_t src_port) {
    VirtualMachine *vm = atoms_hypervisor_get_runtime_vm();
    char buf[1400];
    int p = 0;

    if (!vm || !vm->bsp_vcpu) {
        p = bcat(buf, p, "ERROR: no runtime VM active\n");
        bridge_reply(src_ip, src_port, buf, (uint16_t)p);
        return;
    }

    vCPU *vcpu = vm->bsp_vcpu;
    VirtualPlatform *plat = (VirtualPlatform *)vm->platform;

    /* Guest GPRs from last exit */
    p = bcat(buf, p, "RIP=0x");   p += bridge_hex64(buf + p, vcpu->guest_regs.rip);
    p = bcat(buf, p, " RSP=0x");  p += bridge_hex64(buf + p, vcpu->guest_regs.rsp);
    p = bcat(buf, p, "\nRFLAGS=0x"); p += bridge_hex64(buf + p, vcpu->guest_regs.rflags);
    p = bcat(buf, p, "\nCR0=0x"); p += bridge_hex64(buf + p, vcpu->cr0);
    p = bcat(buf, p, " CR3=0x"); p += bridge_hex64(buf + p, vcpu->cr3);
    p = bcat(buf, p, " CR4=0x"); p += bridge_hex64(buf + p, vcpu->cr4);

    /* Total exits */
    p = bcat(buf, p, "\ntotal_exits=");
    p += bridge_dec(buf + p, vm->total_vmexits);

    /* Last 16 exit reasons with RIP */
    p = bcat(buf, p, "\nlast_exits=[");
    uint32_t n = s_exit_hist.count < ATOMS_BRIDGE_EXIT_HISTORY ? s_exit_hist.count : ATOMS_BRIDGE_EXIT_HISTORY;
    for (uint32_t i = 0; i < n; i++) {
        uint32_t idx = (s_exit_hist.head - n + i + ATOMS_BRIDGE_EXIT_HISTORY) % ATOMS_BRIDGE_EXIT_HISTORY;
        if (i > 0) p = bcat(buf, p, ",");
        p += bridge_dec(buf + p, s_exit_hist.entries[idx].reason);
        p = bcat(buf, p, "@0x");
        p += bridge_hex64(buf + p, s_exit_hist.entries[idx].rip);
    }
    p = bcat(buf, p, "]");

    /* VirtIO IRQ pending */
    p = bcat(buf, p, "\nvirtio_irq_pending=");
    p = bcat(buf, p, vm->virtio_irq_pending ? "1" : "0");

    /* LAPIC / IOAPIC / UART state */
    if (plat) {
        p = bcat(buf, p, "\nlapic_lvt_timer=0x");
        p += bridge_hex32(buf + p, plat->lapic.lvt_timer);

        p = bcat(buf, p, "\nIA32_TSC_DEADLINE=0x");
        p += bridge_hex64(buf + p, plat->lapic.tsc_deadline);

        p = bcat(buf, p, "\nioapic_rte10=0x");
        p += bridge_hex64(buf + p, plat->ioapic.redirection_table[10]);
        p = bcat(buf, p, " rte11=0x");
        p += bridge_hex64(buf + p, plat->ioapic.redirection_table[11]);

        /* PIC state — not emulated separately, report as pata nahi */
        p = bcat(buf, p, "\npic_state=pata nahi");

        /* UART state */
        p = bcat(buf, p, "\nuart_ier=0x");
        p += bridge_hex8(buf + p, plat->uart.ier);
        p = bcat(buf, p, " lsr=0x");
        p += bridge_hex8(buf + p, plat->uart.lsr);
        p = bcat(buf, p, " lcr=0x");
        p += bridge_hex8(buf + p, plat->uart.lcr);
        p = bcat(buf, p, " mcr=0x");
        p += bridge_hex8(buf + p, plat->uart.mcr);
        p = bcat(buf, p, " total_chars=");
        p += bridge_dec(buf + p, plat->uart.total_chars);
    } else {
        p = bcat(buf, p, "\nplatform=pata nahi");
    }

    p = bcat(buf, p, "\n");
    bridge_reply(src_ip, src_port, buf, (uint16_t)p);
}

/* --------------------------------------------------------------------------
 * Command: "mem gpa <addr> <len>"  /  "mem gva <addr> <len>"
 * -------------------------------------------------------------------------- */
static void bridge_cmd_mem(uint32_t src_ip, uint16_t src_port,
                           const char *args) {
    VirtualMachine *vm = atoms_hypervisor_get_runtime_vm();
    if (!vm || !vm->guest_mem || !vm->guest_mem->hva_backing) {
        char e[] = "ERROR: no guest memory\n";
        bridge_reply(src_ip, src_port, e, sizeof(e) - 1);
        return;
    }

    const char *tok; int tl;
    const char *rest = next_token(args, &tok, &tl); /* "gpa" or "gva" */
    bool is_gpa = (tl == 3 && tok[0] == 'g' && tok[1] == 'p' && tok[2] == 'a');

    const char *addr_tok; int addr_len;
    rest = next_token(rest, &addr_tok, &addr_len);
    uint64_t addr = parse_hex_arg(addr_tok);

    const char *len_tok; int len_len;
    next_token(rest, &len_tok, &len_len);
    uint64_t len = parse_dec_arg(len_tok);
    if (len == 0) len = parse_hex_arg(len_tok);
    if (len == 0) len = 64;
    if (len > 256) len = 256;

    GuestMemory *gmem = vm->guest_mem;
    uint8_t *ptr = NULL;

    if (is_gpa) {
        if (addr + len > gmem->gpa_size) {
            char e[] = "ERROR: GPA out of guest RAM range\n";
            bridge_reply(src_ip, src_port, e, sizeof(e) - 1);
            return;
        }
        ptr = (uint8_t *)gmem->hva_backing + addr;
    } else {
        /* GVA translation via existing hypervisor CR3 walk helper */
        if (!vm->bsp_vcpu || !atoms_hypervisor_translate_gva(vm->bsp_vcpu, addr, &ptr) || !ptr) {
            char e[] = "ERROR: GVA translation failed\n";
            bridge_reply(src_ip, src_port, e, sizeof(e) - 1);
            return;
        }
    }

    /* Build hex dump */
    char buf[1400];
    int p = 0;
    p = bcat(buf, p, is_gpa ? "GPA=0x" : "GVA=0x");
    p += bridge_hex64(buf + p, addr);
    p = bcat(buf, p, " len=");
    p += bridge_dec(buf + p, len);
    p = bcat(buf, p, "\n");

    for (uint64_t i = 0; i < len && p < 1350; i++) {
        if ((i & 15) == 0 && i > 0) buf[p++] = '\n';
        p += bridge_hex8(buf + p, ptr[i]);
        buf[p++] = ' ';
    }
    buf[p++] = '\n';
    bridge_reply(src_ip, src_port, buf, (uint16_t)p);
}

/* --------------------------------------------------------------------------
 * Command: "idt <vec>"
 * -------------------------------------------------------------------------- */
static void bridge_cmd_idt(uint32_t src_ip, uint16_t src_port,
                           const char *args) {
    VirtualMachine *vm = atoms_hypervisor_get_runtime_vm();
    if (!vm || !vm->bsp_vcpu) {
        char e[] = "ERROR: no guest\n";
        bridge_reply(src_ip, src_port, e, sizeof(e) - 1);
        return;
    }

    const char *tok; int tl;
    next_token(args, &tok, &tl);
    uint64_t vec = parse_dec_arg(tok);
    if (vec == 0 && tl > 0 && (tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X'))) {
        vec = parse_hex_arg(tok);
    }
    if (vec > 255) { bridge_reply(src_ip, src_port, "ERROR: vec>255\n", 15); return; }

    uint64_t idtr = s_idtr_base;
    if (idtr == 0) idtr = 0x8000; /* Fallback to standard FreeBSD guest IDT GPA */

    uint64_t desc_addr = idtr + (vec * 16);
    uint8_t *desc_hva = NULL;

    /* First try GVA translation via CR3 walk */
    if (!atoms_hypervisor_translate_gva(vm->bsp_vcpu, desc_addr, &desc_hva) || !desc_hva) {
        /* Fallback: direct GPA access in guest RAM */
        if (vm->guest_mem && vm->guest_mem->hva_backing && desc_addr + 16 <= vm->guest_mem->gpa_size) {
            desc_hva = (uint8_t *)vm->guest_mem->hva_backing + desc_addr;
        }
    }

    char buf[256];
    int p = 0;
    p = bcat(buf, p, "idt_vec=");
    p += bridge_dec(buf + p, vec);

    if (desc_hva) {
        uint16_t offset_low = *(uint16_t *)(desc_hva + 0);
        uint16_t selector = *(uint16_t *)(desc_hva + 2);
        uint8_t ist = *(uint8_t *)(desc_hva + 4);
        uint8_t type_attr = *(uint8_t *)(desc_hva + 5);
        uint16_t offset_mid = *(uint16_t *)(desc_hva + 6);
        uint32_t offset_high = *(uint32_t *)(desc_hva + 8);
        uint64_t handler = (uint64_t)offset_low | ((uint64_t)offset_mid << 16) | ((uint64_t)offset_high << 32);

        p = bcat(buf, p, " handler=0x");
        p += bridge_hex64(buf + p, handler);
        p = bcat(buf, p, " sel=0x");
        p += bridge_hex32(buf + p, (uint32_t)selector);
        p = bcat(buf, p, " attr=0x");
        p += bridge_hex8(buf + p, type_attr);
        p = bcat(buf, p, " ist=");
        p += bridge_dec(buf + p, ist);
        p = bcat(buf, p, " idtr=0x");
        p += bridge_hex64(buf + p, idtr);
        p = bcat(buf, p, "\n");
    } else {
        p = bcat(buf, p, " handler=pata nahi\n");
    }
    bridge_reply(src_ip, src_port, buf, (uint16_t)p);
}

/* --------------------------------------------------------------------------
 * Command: "rips" (Send last 256 exit RIPs in MTU-safe chunks)
 * -------------------------------------------------------------------------- */
static void bridge_cmd_rips(uint32_t src_ip, uint16_t src_port) {
    uint32_t total = s_rip_ring.count;
    if (total > ATOMS_BRIDGE_RIP_RING_SIZE) total = ATOMS_BRIDGE_RIP_RING_SIZE;
    if (total == 0) {
        bridge_reply(src_ip, src_port, "rip_count=0\n", 12);
        return;
    }

    uint32_t start = (s_rip_ring.head - total + ATOMS_BRIDGE_RIP_RING_SIZE) % ATOMS_BRIDGE_RIP_RING_SIZE;
    uint32_t sent = 0;

    while (sent < total) {
        char buf[1200];
        int p = 0;
        if (sent == 0) {
            p = bcat(buf, p, "rip_count=");
            p += bridge_dec(buf + p, total);
            p = bcat(buf, p, "\n");
        }
        uint32_t chunk = total - sent;
        if (chunk > 50) chunk = 50;

        for (uint32_t i = 0; i < chunk; i++) {
            uint32_t idx = (start + sent + i) % ATOMS_BRIDGE_RIP_RING_SIZE;
            p = bcat(buf, p, "0x");
            p += bridge_hex64(buf + p, s_rip_ring.rips[idx]);
            p = bcat(buf, p, "\n");
        }
        bridge_reply(src_ip, src_port, buf, (uint16_t)p);
        sent += chunk;
    }
}

/* --------------------------------------------------------------------------
 * Command: "type <text>" (Virtual UART RX Injection)
 * -------------------------------------------------------------------------- */
static void bridge_cmd_type(uint32_t src_ip, uint16_t src_port, const char *text) {
    VirtualMachine *vm = atoms_hypervisor_get_runtime_vm();
    if (!vm || !vm->platform) {
        bridge_reply(src_ip, src_port, "ERROR: no VM platform\n", 22);
        return;
    }
    VirtualPlatform *plat = (VirtualPlatform *)vm->platform;
    uint32_t count = 0;

    while (*text) {
        char c = *text++;
        if (c == '\\' && *text) {
            char nxt = *text++;
            if (nxt == 'n') c = '\n';
            else if (nxt == 'r') c = '\r';
            else if (nxt == 't') c = '\t';
            else if (nxt == '\\') c = '\\';
            else {
                virtual_uart_rx_push(plat, (uint8_t)'\\');
                count++;
                c = nxt;
            }
        }
        if (virtual_uart_rx_push(plat, (uint8_t)c)) {
            count++;
        }
    }

    char buf[64];
    int p = 0;
    p = bcat(buf, p, "OK: typed ");
    p += bridge_dec(buf + p, count);
    p = bcat(buf, p, " chars\n");
    bridge_reply(src_ip, src_port, buf, (uint16_t)p);
}

/* --------------------------------------------------------------------------
 * Command: "reboot"
 * -------------------------------------------------------------------------- */
static void bridge_cmd_reboot(uint32_t src_ip, uint16_t src_port) {
    VirtualMachine *vm = atoms_hypervisor_get_runtime_vm();
    if (!vm) {
        bridge_reply(src_ip, src_port, "ERROR: no VM\n", 13);
        return;
    }
    vm->runtime_active = false;
    vm->shutdown_reason = VM_SHUTDOWN_GUEST_RESET;
    if (vm->bsp_vcpu) vm->bsp_vcpu->state = VM_STATE_STOPPED;
    bridge_reply(src_ip, src_port, "OK: reboot requested\n", 21);
    com1_puts("[BRIDGE] Guest reboot requested via bridge command.\r\n");
}

/* --------------------------------------------------------------------------
 * UDP handler — dispatches incoming commands
 * -------------------------------------------------------------------------- */
static void bridge_udp_handler(uint32_t src_ip, uint16_t src_port,
                               const uint8_t *payload, uint16_t length) {
    if (!payload || length == 0 || length > 512) return;

    /* Null-terminate the command (copy to stack) */
    char cmd[520];
    uint16_t clen = length;
    if (clen > 512) clen = 512;
    memcpy(cmd, payload, clen);
    cmd[clen] = '\0';

    /* Strip trailing newline / carriage return */
    while (clen > 0 && (cmd[clen-1] == '\n' || cmd[clen-1] == '\r')) {
        cmd[--clen] = '\0';
    }

    com1_puts("[BRIDGE RX] ");
    com1_puts(cmd);
    com1_puts("\r\n");

    /* Dispatch */
    if (strcmp(cmd, "state") == 0) {
        bridge_cmd_state(src_ip, src_port);
    } else if (str_startswith(cmd, "mem ")) {
        bridge_cmd_mem(src_ip, src_port, cmd + 4);
    } else if (str_startswith(cmd, "idt ")) {
        bridge_cmd_idt(src_ip, src_port, cmd + 4);
    } else if (strcmp(cmd, "idt") == 0) {
        bridge_cmd_idt(src_ip, src_port, "0");
    } else if (strcmp(cmd, "rips") == 0) {
        bridge_cmd_rips(src_ip, src_port);
    } else if (str_startswith(cmd, "type ")) {
        bridge_cmd_type(src_ip, src_port, cmd + 5);
    } else if (strcmp(cmd, "reboot") == 0) {
        bridge_cmd_reboot(src_ip, src_port);
    } else {
        char err[256];
        int p = 0;
        p = bcat(err, p, "ERROR: unknown command '");
        p = bcat(err, p, cmd);
        p = bcat(err, p, "'\ncommands: state, mem gpa|gva <addr> <len>, idt <vec>, rips, type <text>, reboot\n");
        bridge_reply(src_ip, src_port, err, (uint16_t)p);
    }
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */
void atoms_bridge_init(void) {
    if (s_bridge_active) return;

    memset(&s_rip_ring, 0, sizeof(s_rip_ring));
    memset(&s_exit_hist, 0, sizeof(s_exit_hist));

    if (udp_register_handler(ATOMS_BRIDGE_PORT, bridge_udp_handler)) {
        s_bridge_active = true;
        com1_puts("[BRIDGE] ATOMS Bridge active on UDP port 9998\r\n");
        if (debuglan_active()) {
            debuglan_log_subsys("BRIDGE", "[BRIDGE] ATOMS Bridge registered on UDP port 9998");
        }
        uint32_t gw = netif_get_gateway();
        if (gw != 0) {
            bridge_reply(gw, ATOMS_BRIDGE_PORT, "BRIDGE_ONLINE\n", 14);
        }
    } else {
        com1_puts("[BRIDGE] FAILED to register UDP handler on port 9998!\r\n");
    }
}

void atoms_bridge_record_exit(uint32_t reason, uint64_t rip, uint64_t idtr_base, uint32_t idtr_limit) {
    if (idtr_base != 0) {
        s_idtr_base = idtr_base;
        s_idtr_limit = idtr_limit;
    }

    /* RIP ring */
    s_rip_ring.rips[s_rip_ring.head] = rip;
    s_rip_ring.head = (s_rip_ring.head + 1) % ATOMS_BRIDGE_RIP_RING_SIZE;
    if (s_rip_ring.count < ATOMS_BRIDGE_RIP_RING_SIZE) s_rip_ring.count++;

    /* Exit history */
    s_exit_hist.entries[s_exit_hist.head].reason = reason;
    s_exit_hist.entries[s_exit_hist.head].rip = rip;
    s_exit_hist.head = (s_exit_hist.head + 1) % ATOMS_BRIDGE_EXIT_HISTORY;
    if (s_exit_hist.count < ATOMS_BRIDGE_EXIT_HISTORY) s_exit_hist.count++;
}
