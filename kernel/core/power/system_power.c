#include "system_power.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/net/udp/udp.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern void com1_puts(const char* str);

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

void system_shutdown(void) {
    com1_puts("\r\n[SYSTEM POWER] INITIATING BARE-METAL HARDWARE SHUTDOWN...\r\n");
    display_print("\n[SYSTEM POWER] SHUTTING DOWN ATOMS OS...\n");

    // 1. QEMU ACPI Soft-Off (Port 0x604 / Value 0x2000)
    outw(0x604, 0x2000);

    // 2. VirtualBox / Bochs ACPI Soft-Off (Port 0x404 / Value 0x3400)
    outw(0x404, 0x3400);

    // 3. Intel Haswell H81 PCH ACPI PM1a_CNT S5 Soft-Off (Ports 0x1804, 0x404, 0xB004)
    outw(0x1804, 0x3400);
    outw(0xB004, 0x2000);
    outw(0x600, 0x3400);

    // 4. Intel Haswell PCH ACPI Power State 7 (SLP_TYP=7 | SLP_EN bit 13 -> 0x3C00)
    outw(0x404, 0x3C00);
    outw(0x1804, 0x3C00);

    // 5. APM Power Off (Port 0xB2)
    outb(0xB2, 0x07);

    // If hardware PMIC does not immediately cut ATX VBUS power, halt CPU safely
    com1_puts("[SYSTEM POWER] CPU HALTED — POWER OFF SAFE.\r\n");
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

void system_reboot(void) {
    com1_puts("\r\n[SYSTEM POWER] INITIATING HARDWARE REBOOT...\r\n");
    display_print("\n[SYSTEM POWER] REBOOTING ATOMS OS...\n");

    // 1. Intel PCH Fast Reset Controller (Port 0xCF9 -> Write 0x02, then 0x06)
    // Bit 1 = System Reset, Bit 2 = Hard Reset
    outb(0xCF9, 0x02);
    for (volatile int i = 0; i < 10000; i++);
    outb(0xCF9, 0x06);

    // 2. PS/2 Keyboard Controller Reset Pulse (Port 0x64 -> Command 0xFE)
    outb(0x64, 0xFE);

    // 3. Triple Fault Reset (Load 0-length IDT and trigger interrupt 3)
    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) null_idt = {0, 0};

    __asm__ volatile (
        "cli\n"
        "lidt %0\n"
        "int3\n"
        : : "m"(null_idt)
    );

    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

static void remote_power_udp_callback(uint32_t src_ip, uint16_t src_port, const uint8_t* payload, uint16_t payload_len) {
    (void)src_ip; (void)src_port;
    if (!payload || payload_len == 0) return;

    char cmd[64];
    uint16_t copy_len = (payload_len < 63) ? payload_len : 63;
    memcpy(cmd, payload, copy_len);
    cmd[copy_len] = '\0';

    if (strstr(cmd, "SHUTDOWN") || strstr(cmd, "shutdown")) {
        system_shutdown();
    } else if (strstr(cmd, "REBOOT") || strstr(cmd, "reboot")) {
        system_reboot();
    }
}

void remote_power_init(void) {
    udp_register_handler(9999, remote_power_udp_callback);
    com1_puts("[POWER] Remote UDP Power Management Handler Registered on Port 9999 (REBOOT/SHUTDOWN)\r\n");
}
