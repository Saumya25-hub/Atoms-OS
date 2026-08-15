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

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

#include "kernel/core/pci/pci.h"

typedef enum {
    EFI_RESET_COLD = 0,
    EFI_RESET_WARM = 1,
    EFI_RESET_SHUTDOWN = 2,
    EFI_RESET_PLATFORM_SPECIFIC = 3
} EFI_RESET_TYPE;

typedef void (__attribute__((ms_abi)) *EFI_RESET_SYSTEM_FN)(
    EFI_RESET_TYPE ResetType,
    uint64_t ResetStatus,
    uint64_t DataSize,
    void *ResetData
);

void system_shutdown(void) {
    com1_puts("\r\n[SYSTEM POWER] INITIATING BARE-METAL HARDWARE SHUTDOWN...\r\n");
    display_print("\n[SYSTEM POWER] SHUTTING DOWN ATOMS OS...\n");

    // 0. Immediate Display Blackout (Clean Screen Off)
    extern void rook_blackout_screen(void);
    rook_blackout_screen();

    // 1. UEFI Specification Standard: Runtime Services ResetSystem(EfiResetShutdown)
    // Used by Linux, Ubuntu, and Windows on UEFI LGA1150 / H81 Motherboards
    uint64_t rts_addr = *(uint64_t*)0x1000;
    if (rts_addr != 0 && rts_addr < 0xFFFFFFFFFF000000ULL) {
        uint64_t reset_sys_addr = *(uint64_t*)(rts_addr + 104); // Offset 0x68 = ResetSystem
        if (reset_sys_addr != 0) {
            com1_puts("[SYSTEM POWER] Calling UEFI RuntimeServices->ResetSystem(EfiResetShutdown)...\r\n");
            EFI_RESET_SYSTEM_FN efi_reset = (EFI_RESET_SYSTEM_FN)reset_sys_addr;
            efi_reset(EFI_RESET_SHUTDOWN, 0, 0, 0);
        }
    }

    // 2. Hypervisor ACPI Soft-Off (QEMU / Bochs / VirtualBox)
    outw(0x604, 0x2000); // QEMU
    outw(0x404, 0x3400); // VirtualBox / Bochs

    // 3. Real Intel Haswell H81 PCH Dynamic ACPI S5 Sequence (Linux lpc_ich standard)
    uint32_t vendor_device = pci_read_config_32(0, 31, 0, 0x00);
    uint16_t vendor_id = (uint16_t)(vendor_device & 0xFFFF);

    if (vendor_id == 0x8086) { // Confirmed Genuine Intel PCH (Haswell / LGA1150)
        // Ensure ACPI I/O Decode is enabled in ACPI Control Register (offset 0x44 bit 7)
        uint8_t acpi_cntl = pci_read_config_8(0, 31, 0, 0x44);
        if (!(acpi_cntl & 0x80)) {
            pci_write_config_8(0, 31, 0, 0x44, (uint8_t)(acpi_cntl | 0x80));
        }

        uint32_t pmbase_reg = pci_read_config_32(0, 31, 0, 0x40);
        uint16_t pmbase = (uint16_t)(pmbase_reg & 0xFF80);
        if (pmbase >= 0x0400 && pmbase <= 0xFF00) {
            uint16_t pm1_sts = pmbase + 0x00;
            uint16_t pm1_cnt = pmbase + 0x04;
            uint16_t smi_en  = pmbase + 0x30;

            // Clear WAK_STS (bit 15) and all pending status flags
            outw(pm1_sts, (uint16_t)0xFFFF);

            // Send ACPI_ENABLE (0xA0 / 0x01) to ACPI SMI command port
            outb(0xB2, 0xA0);
            outb(0xB2, 0x01);

            // Disable SMM SMI sleep interception (allows hardware PMIC to cut power directly)
            outl(smi_en, 0x00000000);

            // Read current PM1_CNT register
            uint16_t cnt_val = io_in16(pm1_cnt);

            // Set SLP_TYP = 7 (S5 Soft-Off) and SLP_EN (bit 13)
            uint16_t s5_cmd = (cnt_val & ~(7 << 10)) | (7 << 10) | (1 << 13);
            outw(pm1_cnt, s5_cmd);

            for (volatile int i = 0; i < 50000; i++) { __asm__ volatile("pause"); }

            // Try SLP_TYP = 5 (alternative S5 encoding on some BIOS vendors)
            s5_cmd = (cnt_val & ~(7 << 10)) | (5 << 10) | (1 << 13);
            outw(pm1_cnt, s5_cmd);

            for (volatile int i = 0; i < 50000; i++) { __asm__ volatile("pause"); }

            // Try standard S5 state 0x3C00
            outw(pm1_cnt, (uint16_t)0x3C00);

            for (volatile int i = 0; i < 50000; i++) { __asm__ volatile("pause"); }

            // Try standard S5 state 0x3400
            outw(pm1_cnt, (uint16_t)0x3400);
        }
    }

    // 4. Safe Bare-Metal CPU Halt (Screen is 100% black, 5VSB rail protected)
    com1_puts("[SYSTEM POWER] CPU HALTED — POWER OFF SAFE.\r\n");
    display_print("[SYSTEM POWER] System Halted Safely. It is now safe to turn off your computer.\n");
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

void system_reboot(void) {
    com1_puts("\r\n[SYSTEM POWER] INITIATING HARDWARE REBOOT...\r\n");
    display_print("\n[SYSTEM POWER] REBOOTING ATOMS OS...\n");

    // 0. UEFI Specification Standard: Runtime Services ResetSystem(EfiResetCold)
    uint64_t rts_addr = *(uint64_t*)0x1000;
    if (rts_addr != 0 && rts_addr < 0xFFFFFFFFFF000000ULL) {
        uint64_t reset_sys_addr = *(uint64_t*)(rts_addr + 104);
        if (reset_sys_addr != 0) {
            com1_puts("[SYSTEM POWER] Calling UEFI RuntimeServices->ResetSystem(EfiResetCold)...\r\n");
            EFI_RESET_SYSTEM_FN efi_reset = (EFI_RESET_SYSTEM_FN)reset_sys_addr;
            efi_reset(EFI_RESET_COLD, 0, 0, 0);
        }
    }

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
